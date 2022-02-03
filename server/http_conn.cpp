#include"http_conn.hpp"
const char* ok_200_title = "OK\r\n";
const char* error_400_title = "BAD Request\r\n";
const char* error_400_form = "Your request has bad syntax or is inherently impossible to satisfy.\r\n";
const char* error_403_title = "Forbidden\r\n";
const char* error_403_form = "You don not have permission to get file from this server.\r\n";
const char* error_404_title = "Not Found\r\n";
const char* error_404_form = "The requested file was not found on this server.\r\n";
const char* error_500_title = "Internal Error";
const char* error_500_form = "There was an unusual problem serving the requested file.\r\n";

const char* doc_root = "/var/www/html"; //网站根目录
int 
setnonblocking(int fd) {
    int old_op = fcntl(fd, F_GETFL);
    int new_op = old_op | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_op);
    return old_op;
}
void 
addfd(int epollfd, int fd, bool one_shot) {
    epoll_event eve;
    eve.events = EPOLLIN | EPOLLET;
    if(one_shot) eve.events |= EPOLLONESHOT;
    eve.data.fd = fd;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &eve);
    setnonblocking(fd);
}
void 
removefd(int epollfd, int fd) {
    epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, NULL);
    close(fd);
}
void 
modfd(int epollfd, int fd, int ev) {
    epoll_event event;
    event.data.fd = fd;
    event.events = ev | EPOLLET | EPOLLONESHOT | EPOLLRDHUP;
    epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &event);
}

int http_conn::m_user_count = 0;
int http_conn::m_epollfd = -1;

void 
http_conn:: close_conn(bool real_close) {
    if(real_close && m_sockfd != -1) {
        removefd(m_epollfd, m_sockfd); //remove将关闭这个fd
        m_sockfd = -1;
        m_user_count--;
    }

}

void 
http_conn::init(int sockfd, const sockaddr_in& addr) {
    m_sockfd = sockfd;
    m_address = addr;
    /*还可以添加 避免time_wait的代码，setsockopt(so_reuseaddr)*/
    addfd(m_epollfd, sockfd, true);
    m_user_count++;
    init();
}
void 
http_conn::init() {

    m_check_state = CHECK_STATE_REQUESTLINE; //初始时主状态机 在请求行状态
    m_linger = false;
    m_method = GET; // here
    m_url = 0;
    m_version = 0;
    m_content_length = 0;
    m_host = 0;
    m_start_line = 0;
    m_checked_idx = 0;
    m_read_idx = 0;
    m_write_idx = 0;
    memset(m_read_buf, '\0', READ_BUFFER_SIZE);
    memset(m_write_buf, '\0', WRITE_BUFFER_SIZE);
    memset(m_real_file, '\0', FILENAME_LEN);
    
}
http_conn:: LINE_STATUS 
http_conn:: parse_line() { //解析出一行，从本行首开始
    char temp;
    for(; m_checked_idx < m_read_idx; ++ m_checked_idx) {
        temp = m_read_buf[m_checked_idx];
        if(temp == '\r') {
            if(m_read_idx == m_checked_idx + 1)
                return LINE_OPEN; //\r 以及到了末尾

            else if(m_read_buf[m_checked_idx + 1] == '\n') { //把\r\n覆盖
                m_read_buf[m_checked_idx++] = '\0';
                m_read_buf[m_checked_idx++] = '\0';
                return LINE_OK;
            }
            return LINE_BAD;

        }
        else if(temp == '\n') { //上次是LINE_OPEN,这次继续解析
            if(m_checked_idx > 1 && m_read_buf[m_checked_idx - 1] == '\r') {
                m_read_buf[m_checked_idx - 1] = '\0';
                m_read_buf[m_checked_idx++] = '\0';
                return LINE_OK;
            }

            return LINE_BAD;
        }       
    }
    return LINE_OPEN;
}

bool 
http_conn:: read() {
    if(m_read_idx >= READ_BUFFER_SIZE) {
        return false;
    }
    int bytes_read = 0;
    while(true) { //循环，读至无数据或者对方关闭连接
        bytes_read = recv(m_sockfd, m_read_buf + m_read_idx, READ_BUFFER_SIZE - m_read_idx, 0);
        if(bytes_read == -1) {
            if(errno == EAGAIN || errno == EWOULDBLOCK) { //读取完毕
                break;
            }
            return false; //发生错误
        }
        else if(bytes_read == 0) { //对方关闭连接
            return false;
        }
        m_read_idx += bytes_read;
    }
    return true;
}

http_conn::HTTP_CODE 
http_conn:: parse_request_line(char* text) { //正确格式应为： 请求字段 URL 版本号
    //解析请求行
    m_url= strpbrk(text, " "); //strpbrk是查找字符函数，返回位置
    if(!m_url) { //
        return BAD_REQUEST;  
    }
     *m_url++ = '\0';
    char*method = text;
    if(strcasecmp(method, "GET") == 0) { //忽略大小写比较字符串
        m_method = GET;
    }
    else {
        return BAD_REQUEST;
    }
    m_url += strspn(m_url, " "); //向后偏移
    m_version = strpbrk(m_url, " ");
    if(!m_version) {
        return BAD_REQUEST;
    }
    *m_version++ = '\0';
    m_version += strspn(m_version, " "); //strspn：从开头开始的出现的字符数
    if(strcasecmp(m_version, "HTTP/1.1") != 0) {
        return BAD_REQUEST;
    }
    if(strncasecmp(m_url, "http://",7) == 0) {
        m_url += 7; //跳过http://
        m_url = strchr(m_url, '/');//指向 /之后的资源
    }
    if(!m_url || m_url[0] != '/') {
        return BAD_REQUEST;
    }
    m_check_state = CHECK_STATE_HEADER; //主状态机跳到下一个状态
    return NO_REQUEST; //未完成，进入下一个状态

}

http_conn:: HTTP_CODE http_conn::parse_headers(char*text) {
    if(text[0] == '\0') { //遇到空行，表示头部字段解析完毕(请求头是有多个行的，需要多次调用)

        if(m_content_length != 0) {
        /*
        如果http请求有消息体，则还需要读取m_content_length字节的消息体
        状态机转移到CHECK_STATE_CONTENT状态
        */
            m_check_state = CHECK_STATE_CONTENT;
            return NO_REQUEST;
        }
        return GET_REQUEST; //已经得到了完整的HTTP请求
    }
    else if(strncasecmp(text, "Connection:",11) == 0) {
        text += 11;
        text += strspn(text, " ");
        if(strcasecmp(text, "keep-alive") == 0) {
            m_linger = true;
        }
    }
    else if(strncasecmp(text, "Content-Length:", 15) == 0) {
        text += 15;
        text += strspn(text, " ");
        m_content_length = atol(text);
    }
    else if(strncasecmp(text, "Host:", 5) == 0) {
        text += 5;
        text += strspn(text, " ");
        m_host = text;
    } 
    else {
        printf("unknown header %s\n", text);
    }
    return NO_REQUEST;
}

http_conn:: HTTP_CODE http_conn:: parse_content(char*text) {
    if(m_read_idx >= m_content_length + m_checked_idx) { //只检验长度，内容不检验
        text[m_content_length] = '\0';
        return GET_REQUEST;
    }
    return NO_REQUEST;
}

http_conn:: HTTP_CODE http_conn:: process_read() {
    /* 主状态机 */
    LINE_STATUS line_status = LINE_OK;
    HTTP_CODE ret = NO_REQUEST;
    char* text;
    while((m_check_state == CHECK_STATE_CONTENT && line_status == LINE_OK)
    || (line_status = parse_line()) == LINE_OK) {
        text = get_line(); //获取行首位置
        m_start_line = m_checked_idx; //定位至下次行首位置
        printf("Got 1 http line: %s\n", text);
       
        switch(m_check_state) {
            
            case CHECK_STATE_REQUESTLINE: {
               // puts("request line");
                ret = parse_request_line(text); //去解析请求行
               // puts("request line");
                if(ret == BAD_REQUEST) 
                    return BAD_REQUEST;
                 puts("request line");    
                break;

            }
            case CHECK_STATE_HEADER: {
               // puts("request headers");
                ret = parse_headers(text); //去分析请求头
                
                if(ret == BAD_REQUEST)
                    return BAD_REQUEST;
                else if(ret == GET_REQUEST) { //请求完整，分析文件
                    return do_request();
                }
                line_status = LINE_OPEN;
                break;
            }
            case CHECK_STATE_CONTENT: {
               // puts("xxx---yyy");
                ret = parse_content(text);
                if(ret == GET_REQUEST)
                    return do_request();
                line_status = LINE_OPEN;
                break;    
            }
            default: {
                return INTERNAL_ERROR;
            }
        }
   
    }
   // puts("xxx---yyy-");
    return NO_REQUEST;
}

http_conn:: HTTP_CODE http_conn:: do_request() {
    /* 获得完整http请求后,就分析目标文件属性，进行映射
    */
   char* pwd = "/root/webproj/dirfile"; // 文件所在目录 
   strcpy(m_real_file, pwd);
   int len = strlen(pwd);
   strncpy(m_real_file + len, m_url, FILENAME_LEN - len - 1); //m_real_file为完整文件名路径
   if(stat(m_real_file, &m_file_stat) < 0) {
       return NO_RESOURCE;
   } 
   if(!m_file_stat.st_mode&S_IROTH) { //其他组读权限
       return FORBIDDEN_REQUEST;
   }
   if(S_ISDIR(m_file_stat.st_mode)) { //是目录
       return BAD_REQUEST;
   }
   int fd = open(m_real_file, O_RDONLY); //打开文件
   m_file_address = (char*)mmap(NULL, m_file_stat.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
   //映射文件初始地址
   close(fd);
   return FILE_REQUEST; //ok 

}

void http_conn:: unmap() {
    if(m_file_address) {
        munmap(m_file_address, m_file_stat.st_size);
        m_file_address = NULL;
    }
}
bool http_conn::write() { //内核缓冲区就绪，可以写
    int temp = 0;
    int bytes_have_send = 0;
    int bytes_to_send = m_write_idx;
    if(bytes_to_send == 0) { //没有要发送的
        modfd(m_epollfd, m_sockfd, EPOLLIN); //开始监听读
        init();
        return true;
    }   
    while(true) {
        temp = writev(m_sockfd, m_iv, m_iv_count);
        if(temp <= -1) {
            if(errno == EAGAIN) { //内核满，等待下次
                modfd(m_epollfd, m_sockfd, EPOLLOUT);
                return true;
            }
            unmap();
            return false;
        }
        bytes_to_send -= temp;
        bytes_have_send += temp;
        if(bytes_to_send <= bytes_have_send) {
            unmap();
            if(m_linger) { //发送完毕，决定是否关闭连接
                init();
                modfd(m_epollfd, m_sockfd, EPOLLIN);
                return true;
            }
            else { //关闭连接
                modfd(m_epollfd, m_sockfd, EPOLLIN);
                return false;
            }
        }
    }
}
bool http_conn::add_response(const char* format, ...) {//格式化字符串与后续参数
    if(m_write_idx >= WRITE_BUFFER_SIZE) {
        return false;
    }
    va_list arg_list;
    va_start(arg_list, format);
    int len = vsnprintf(m_write_buf + m_write_idx, WRITE_BUFFER_SIZE - 1 - m_write_idx, format, arg_list);
    if(len >= WRITE_BUFFER_SIZE - 1 - m_write_idx) {
        return false;
    }
    m_write_idx += len;
    va_end(arg_list);
    return true;
}
bool http_conn::add_status_line(int status, const char* title) {
    return add_response("%s %d %s\r\n", "HTTP/1.1", status, title);
}
bool http_conn::add_headers(int content_len) {
    add_content_length(content_len);
    add_linger();
    return add_blank_line();
}
bool http_conn:: add_content_length(int content_len) {
    return add_response("Content-Length:%d\r\n",content_len);
}
bool http_conn::add_linger() {
    return add_response("Connection:%s\r\n",
    (m_linger) ? "keepalive" : "close");
}
bool http_conn::add_blank_line() {
    return add_response("%s", "\r\n");
}
bool http_conn::add_content(const char* content) {
    return add_response("%s", content);
}

bool http_conn::process_write(HTTP_CODE ret) {
    switch(ret) {
        case INTERNAL_ERROR: {
            add_status_line(500, error_500_title);
            add_headers(strlen(error_500_form));
            if(!add_content(error_500_form)) 
                return false;
            break;    
        }
        case BAD_REQUEST: {
            add_status_line(400, error_400_title);
            add_headers(strlen(error_400_form));
            if(!add_content(error_400_form)) 
                return false;
            break;    
        }
        case NO_RESOURCE: {
            add_status_line(404, error_404_title);
            add_headers(strlen(error_404_form));
            if(!add_content(error_404_form)) 
                return false;
            break;    
        }
        case FORBIDDEN_REQUEST: {
            add_status_line(403, error_403_title);
            add_headers(strlen(error_403_form));
            if(!add_content(error_403_form)) 
                return false;
            break;    
        }
        case FILE_REQUEST: {
            add_status_line(200, ok_200_title);
            if(m_file_stat.st_size != 0) {
                add_headers(m_file_stat.st_size);
                m_iv[0].iov_base = m_write_buf;
                m_iv[0].iov_len = m_write_idx;
                m_iv[1].iov_base = m_file_address;
                m_iv[1].iov_len = m_file_stat.st_size;
                m_iv_count = 2;
                //http报文和文件内容，两个缓冲区连在一起
                return true;
            }
            else {
                const char* ok_string = "<html><body></body></html>";
                add_headers(strlen(ok_string));
                if(!add_content(ok_string)) {
                    return false;
                }
            }
        }
        default: {
            return false;
        }
        
    }
    m_iv[0].iov_base = m_write_buf;
    m_iv[0].iov_len = m_write_idx;
    m_iv_count = 1;
    return true;
}

void http_conn::process() {
    HTTP_CODE read_ret = process_read();
    if(read_ret == NO_REQUEST) {
        modfd(m_epollfd, m_sockfd, EPOLLIN);
        return;
    }

    bool write_ret = process_write(read_ret);
    printf("%s\n",write_ret?"Yep":"No");
    if(!write_ret) {
       
        close_conn();
    }
    modfd(m_epollfd, m_sockfd, EPOLLOUT); //准备写入
}







