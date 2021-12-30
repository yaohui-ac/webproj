#ifndef HTTPCONNECTION_HPP
#define HTTPCONNECTION_HPP
#include<unistd.h>
#include<signal.h>
#include<sys/types.h>
#include<sys/epoll.h>
#include<fcntl.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<sys/stat.h>
#include<string.h>
#include<pthread.h>
#include<stdio.h>
#include<pthread.h>
#include<stdlib.h>
#include<sys/mman.h>
#include<stdarg.h>
#include<errno.h>
#include<sys/uio.h>
#include"lock.hpp"

class http_conn {
    public:
        static int m_epollfd; //所有socket上的事件都被注册到同一个epoll中
        static int m_user_count; //统计用户数量
        static const int FILENAME_LEN = 200; //文件名最大长度
        static const int READ_BUFFER_SIZE = 2048; //读缓冲区
        static const int WRITE_BUFFER_SIZE = 1024; //写缓冲区
        enum METHOD { // 仅支持get方法，
            GET = 0, POST, HEAD, PUT, DELETE, 
            TRACE, OPTIONS, CONNECT, PATCH 
        };
        enum CHECK_STATE { //解析客户请求时，主状态机状态
            CHECK_STATE_REQUESTLINE = 0, //分析请求行
            CHECK_STATE_HEADER, //分析请求头部字段
            CHECK_STATE_CONTENT //分析内容
        };
        enum HTTP_CODE { //服务器处理http请求可能的结果
            NO_REQUEST,//请求不完整，需要继续读取客户数据
            GET_REQUEST,//获得了完整的客户请求
            BAD_REQUEST, //客户请求有语法错误
            NO_RESOURCE, //404无资源
            FORBIDDEN_REQUEST, //客户对资源无足够访问权限
            FILE_REQUEST, 
            INTERNAL_ERROR,// 服务器内部错误
            CLOSED_CONNECTION//客户端已经关闭连接
        };
        enum LINE_STATUS {
            LINE_OK = 0, //读取了完整行
            LINE_BAD,  //行出错
            LINE_OPEN //不完整
        };

    private:
        void init(); //初始化连接
        HTTP_CODE process_read(); //解析http请求
        bool process_write(HTTP_CODE ret); //填充http应答
        HTTP_CODE parse_request_line(char*text); /*将被用于分析http请求*/
        HTTP_CODE parse_headers(char*text); /*将被用于分析http请求*/
        HTTP_CODE parse_content(char*text);
        HTTP_CODE do_request(); /*将被用于分析http请求*/
        char* get_line() {  return m_read_buf + m_start_line; }; /*将被用于分析http请求*/
        LINE_STATUS parse_line();
        /*以下函数将被 process_write调用，填充HTTP应答*/
        void unmap();
        bool add_response(const char* format, ...);
        bool add_content(const char* content);
        bool add_status_line(int status, const char* title);
        bool add_headers(int content_length);
        bool add_content_length(int content_length);
        bool add_linger();
        bool add_blank_line();
        int m_sockfd; //连接的socket
        sockaddr_in m_address; //对方的socket地址
        char m_read_buf[READ_BUFFER_SIZE]; //读缓冲区
        int m_read_idx; //读缓冲区中 最后一个字节的下一个位置
        int m_checked_idx; //正在解析的字符在读缓冲区的位置
        int m_start_line; //正在解析的行 的起始位置
        char m_write_buf[WRITE_BUFFER_SIZE]; //写缓冲区
        int m_write_idx; //待发送字节数
        CHECK_STATE m_check_state; //主状态机位置
        METHOD m_method; //请求方法，姑且支持GET
        char m_real_file[FILENAME_LEN]; //客户请求的文件名的完整路径 doc_root + m_url
        char* m_url;  //用户请求的文件名
        char* m_version; //http协议版本号，只支持1.1
        char* m_host; //主机名
        int m_content_length; //http请求的消息体长度
        bool m_linger; // http请求是否要保持连接
        char* m_file_address; //目标文件被映射到内存中的起始位置
        struct stat m_file_stat;//目标文件的状态，可以判断文件是否存在，是否为目录等
        struct iovec m_iv[2]; // 采用writev执行写，于是定义如下成员，内存块
        int m_iv_count;

    public:
        http_conn() {}
        ~http_conn() {}
        void init(int sockfd, const sockaddr_in &addr);//初始化新接收的连接
        void close_conn(bool real_close = true);//关闭连接
        void process();//处理客户请求
        bool read();//非阻塞读
        bool write();//非阻塞写

};
#endif