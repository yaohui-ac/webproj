#include<sys/socket.h>
#include<sys/types.h>
#include<sys/epoll.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<stdio.h>
#include<assert.h>
#include<unistd.h>
#include<errno.h>
#include<string.h>
#include<fcntl.h>
#include<stdlib.h>
#include<string>
#include<iostream>
namespace http_state {
    int httpstatus;
    bool keepalive;
    int file_size;
};
using http_state:: httpstatus;
const char* filename;
int ffd;
char* get_mess[5] = {
    "GET http://service/settings.txt HTTP/1.1\r\n",
    "Host: www.rt-thread.com\r\n",
    "Connection: keep-alive\r\n",
    "Content-Length: 0\r\n",
    "\r\n"
};
 enum LINE_STATUS {
    LINE_OK = 0, //读取了完整行
    LINE_BAD,  //行出错
    LINE_OPEN //不完整
};
enum CHECK_STATE { //解析客户请求时，主状态机状态
    CHECK_STATE_STATUSLINE = 0, //分析请求行
    CHECK_STATE_HEADER, //分析请求头部字段
    CHECK_STATE_CONTENT //分析内容
};
enum PARSE_HTTP {
    PARSE_OK = 0,
    PARSE_ERROR
};
char buf[1024 * 512];
int buf_size = 1024 * 512;

LINE_STATUS get_line(int& parse_idx, int & read_idx) {
    for( ;parse_idx <= read_idx; parse_idx++) {
        if(buf[parse_idx] == '\r') {
            if(parse_idx == read_idx)
                return LINE_OPEN;
            if(buf[parse_idx + 1] == '\n') {
                buf[parse_idx++] = '\0';
                buf[parse_idx++] = '\0';
                return LINE_OK;
            }    
            return LINE_BAD;
        }
        else if(buf[parse_idx] == '\n') {
            if(buf[parse_idx - 1] != '\r')
                return LINE_BAD;
            buf[parse_idx - 1] = '\0';
            buf[parse_idx] = '\0';    
            return LINE_OK;
        }       
    }
    return LINE_OPEN;
}
PARSE_HTTP parse_statusline(const char* line) {
// HTTP1.1 200 message\r\n
   std::string str(line);
   std::size_t found = 0;
   found = str.find("HTTP1.1"); // 0 -> 
   if (found == std::string::npos) {
       puts("status line error: NOT HTTP1.1");
       return PARSE_ERROR;
   }
   found += 8;
   for(int i = 0; i < 3; i++) {
       if(str[i + found] < '0' || str[i + found] > '9') {
          puts("status line error: STATUS CODE ERROR");
          return PARSE_ERROR;
       }
   }
   httpstatus = std::stoi(str.substr(found, 3));
   std:: cout << str << std::endl;
   return PARSE_OK;
}
PARSE_HTTP parse_header(const char* line, int cnt) {
    /*
    Content-Length:xxx\r\n
    Connection:keepalive\r\n
    \r\n
    */
   std::string str(line);
   if(cnt == 1) {
       if(str.find("Content-Length:") == std::string::npos) {
            puts("header error: no content length");
            return PARSE_ERROR;
    }
       http_state::file_size = atoi(line + 15);
   }

   else if(cnt == 2) {
       if(str.find("Connection:") == std::string::npos) {
            puts("header error: no connection");
            return PARSE_ERROR;
        }
        http_state::keepalive = (str[11] == 'k');
        return PARSE_OK;
   }
   else {
       return PARSE_OK;
   }
   
}
PARSE_HTTP parse_content(const char* line) {
    if(http_state::httpstatus != 200) {
        puts(line);
        return PARSE_OK;
    }
}
void parse(int fd) {
    int read_idx = 0;
    int parse_idx = 0;
    int line_begin = 0;
    int len = -1;
    int write_idx = 0;
    CHECK_STATE state = CHECK_STATE_STATUSLINE;
    while((len = read(fd, buf + read_idx, buf_size - read_idx)) >= 0) {
        if(len == 0) {
            puts("Remote connection Closed");
            break;
            // 处理客户连接关闭的情况
        }
        read_idx += len;
        if(state == CHECK_STATE_CONTENT) {
            if(http_state::httpstatus == 200) {
                int wed = write(ffd, buf + write_idx, read_idx - write_idx);
                write_idx += wed;
            }
        }
        
        LINE_STATUS line_status = LINE_OPEN;
        line_status = get_line(parse_idx, read_idx);
        if(line_status == LINE_OPEN) {
            continue; //继续读取
        }
        else if(line_status == LINE_BAD) {
            // 处理line_bad
        }
      
        switch(state) {
            case CHECK_STATE_STATUSLINE: {
                PARSE_HTTP parse_state = parse_statusline(buf + line_begin);
                if(parse_state == PARSE_ERROR) {
                    // 错误处理程序
                }
                state = CHECK_STATE_HEADER;
                break;
            }
            case CHECK_STATE_HEADER: {
                 static int  cnt = 1;
                 PARSE_HTTP parse_state = parse_header(buf + line_begin, cnt);
                 if(parse_state == PARSE_ERROR) {
                    // 错误处理程序
                }
                cnt++;
                if(cnt == 4) {
                    state = CHECK_STATE_CONTENT;
                    write_idx = parse_idx;
                }
                   
                break;
            }
            case CHECK_STATE_CONTENT: {

                break;
            }
        }
        
        line_begin = parse_idx;
    }

}
int main (int argc, char* argv[]) {
    if(argc <= 3) {
        puts("paremeter error");
        return 1;
    }
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    const char* ip = argv[1];
    int port = atoi(argv[2]);
    filename = argv[3];
    ffd = open(filename, O_CREAT|O_RDWR);
    assert(fd >= 0);
    sockaddr_in address;
    bzero(&address, sizeof address);
    address.sin_port = htons(port);
    address.sin_family = AF_INET;
    inet_pton(AF_INET, ip , &address.sin_addr);
    socklen_t addrlen = sizeof address;
    int ret = connect(fd, (sockaddr*)&address, addrlen);
    assert(ret != -1);
  
    for(int i = 0; i < 5; i++) {
        int len = write(fd, get_mess[i], strlen(get_mess[i]));
        if(len <= 0) 
            puts("write error");
    }

    parse(fd);

    return 0;
}