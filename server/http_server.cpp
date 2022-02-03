#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<stdio.h>
#include<unistd.h>
#include<errno.h>
#include<string.h>
#include<fcntl.h>
#include<stdlib.h>
#include<assert.h>
#include<sys/epoll.h>
#include"initserver.hpp"
#include"lock.hpp"
#include"threadpool.hpp"
#include"http_conn.hpp"
#define MAX_FD 65536
#define MAX_EVENT_NUMBER 10000
extern int addfd(int epollfd, int fd, bool one_shot);
extern int removefd(int epollfd, int fd);
using server_config::listenfd; // 监听套接字

void addsig(int sig, void(handler)(int), bool restart = true) {
    struct sigaction sa;
    memset(&sa, 0, sizeof sa);
    sa.sa_handler = handler;
    if(restart) {
        sa.sa_flags|= SA_RESTART;
    }
    sigfillset(&sa.sa_mask);
    assert(sigaction(sig, &sa, NULL) != -1);
}
void show_error(int connfd, const char* info) {
    printf("%s", info);
    send(connfd, info, strlen(info), 0);
    close(connfd);
}

int main(int argc, char* argv[]) {
   
    addsig(SIGPIPE, SIG_IGN);
    server_config::init_server(argc, argv);
    threadpool<http_conn>* pool = NULL;
    try {
        pool = new threadpool<http_conn>; 
    }
    catch(...) {
        return 1;
    }
    
    http_conn* users = new http_conn[MAX_FD];
    //预先分配客户连接的资源，缓冲区等
    assert(users);
    int user_count = 0;
    
    epoll_event events[MAX_EVENT_NUMBER];
    int epollfd = epoll_create(5);
    assert(epollfd != -1);
    http_conn::m_epollfd = epollfd;
    addfd(epollfd, listenfd, false);
    while(true) {
        int number = epoll_wait(epollfd, events, MAX_EVENT_NUMBER,  -1);
        if(number < 0 && errno != EINTR) {
            printf("epoll failure\n");
            break;
        }
        for(int i = 0; i < number; i++) {
            int sockfd = events[i].data.fd;
            if(sockfd == listenfd) {
                sockaddr_in client_address;
                socklen_t client_addrlength = sizeof client_address;
                int connfd = accept(listenfd, (sockaddr*)& client_address, &client_addrlength);
                if(connfd < 0) {
                    printf("Errno is %s\n", strerror(errno));
                    continue;
                }
                if(http_conn::m_user_count >= MAX_FD) {
                    show_error(connfd, "Internal server busy");
                    continue;
                }
                users[connfd].init(connfd, client_address);
            }

            else if(events[i].events&(EPOLLRDHUP|EPOLLHUP|EPOLLERR)) {
                users[sockfd].close_conn(); // 关闭连接,出现了错误
            }

            else if(events[i].events & EPOLLIN) {
                if(users[sockfd].read()) {
                    pool->append(users + sockfd); //丢入队列
                }
                else {
                    users[sockfd].close_conn();
                }
            }

            else if(events[i].events & EPOLLOUT) {
                if(!users[sockfd].write()) { //false，关闭连接
                    users[sockfd].close_conn();
                }
            }

            else {

            }
        }
    }
    
    close(epollfd);
    close(listenfd);
    delete[]users;
    delete pool;
    return 0;

}