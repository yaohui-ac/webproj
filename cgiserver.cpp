#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<assert.h>
#include<stdio.h>
#include<unistd.h>
#include<errno.h>
#include<string.h>
#include<fcntl.h>
#include<sys/epoll.h>
#include<signal.h>
#include<sys/wait.h>
#include<sys/stat.h>
#include"processpool.hpp"

class cgi_conn { 
    //处理CGI请求的类，将作为processpool类的模板参数
    private:
        static const int BUFFER_SIZE = 1024;
        static int m_epollfd;
        int m_sockfd;
        sockaddr_in m_address;
        char m_buf[BUFFER_SIZE];
        int m_read_idx;//标记读的位置
    public:
        cgi_conn(){}
        ~cgi_conn(){}

        void init(int epollfd, int sockfd, const sockaddr_in&client_addr) {
            //初始化客户连接
            m_epollfd = epollfd;
            m_sockfd = sockfd;
            m_address = client_addr;
            memset(m_buf, 0, BUFFER_SIZE);
            m_read_idx = 0;
        }
        void process() {
            int idx = 0;
            int ret = -1;
            while(true) {
                idx = m_read_idx;
                ret = recv(m_sockfd, m_buf + idx, BUFFER_SIZE - 1 - idx, 0);
                if(ret < 0) {
                    if(errno != EAGAIN) {
                        removefd(m_epollfd, m_sockfd);
                    }
                    break;
                }
                else if(ret == 0) {
                    removefd(m_epollfd, m_sockfd);
                    break;
                }
                else {
                    m_read_idx += ret;
                    printf("User content is %s\n", m_buf);
                    for(; idx < m_read_idx; ++idx) {
                        if(idx >= 1 && m_buf[idx - 1] == '\r' && m_buf[idx] == '\n') {
                            break;
                            //遇到\r\n开始处理客户数据
                        }
                    }
                    if(idx == m_read_idx) { //需要读取更多客户数据
                        continue;
                    }
                    char* file_name = m_buf;
                    if(access(file_name, F_OK) == -1) {
                        //判断客户要运行的CGI程序是否存在
                        removefd(m_epollfd, m_sockfd);
                        break;
                    }
                    ret = fork();
                    if(ret == -1) {
                        removefd(m_epollfd, m_sockfd);
                        break;
                    }
                    else {
                        close(STDOUT_FILENO);
                        dup(m_sockfd);
                        execl(m_buf, m_buf, NULL);
                        exit(0);
                    }
            
                }

            }
        }
};
int cgi_conn::m_epollfd = -1;

int main(int argc, char *argv[]) {
    if(argc <= 2) {
        printf("Usuage: %s ip_address port_number\n", basename(argv[0]));
        return 1;
    }
    const char*ip = argv[1];
    int port = atoi(argv[2]);
    int listenfd = socket(PF_INET, SOCK_STREAM, 0);
    assert(listenfd >= 0);
    int ret = 0;
    sockaddr_in address;
    bzero(&address, sizeof address);
    address.sin_family = AF_INET;
    ret = bind(listenfd, (sockaddr*)&address, sizeof address);
    assert(ret != -1);
    ret = listen(listenfd, 5);
    assert(ret != -1);
    processpool<cgi_conn>* pool = processpool<cgi_conn>::create(listenfd);
    //进程池对象，中间应是内含多个进程
    if(pool) {
        pool->run();
        delete pool;
    }
    close(listenfd);
    return 0;
}

