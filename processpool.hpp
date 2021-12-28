#ifndef PROCESSPOOL_HPP
#define PROCESSPOOL_HPP
#include<sys/socket.h>
#include<sys/types.h>
#include<sys/wait.h>
#include<sys/stat.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<stdio.h>
#include<assert.h>
#include<time.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include<sys/epoll.h>
#include<pthread.h>
#include<errno.h>
#include<fcntl.h>
#include<signal.h>

class process {
    public:
        pid_t m_pid;
        int m_pipefd[2];

    public:
    process():m_pid(-1)
    {   }
};
template<typename T>
class processpool {
    private:
        static const int MAX_PROCESS_NUMBER = 16;
        static const int USER_PER_PROCESS = 65536;
        static const int MAX_EVENT_NUMBER = 10000;
        int m_process_number; //进程池进程数目
        int m_idx; //进程编号
        int m_epollfd; //每个进程都有一个监听
        int m_listenfd; //侦听
        int m_stop; //是否停止
        process* m_sub_process; //子进程的描述信息
        static processpool<T>* m_instance; //静态实例
        void setup_sig_pipe();
        void run_parent();
        void run_child();
    private:
        processpool(int listenfd, int process_number = 8);
    public:
        static processpool<T>* create(int listenfd, int process_number) {
            if(!m_instance) { //懒汉模式
                m_instance = new processpool<T>(listenfd, process_number);
            }
            return m_instance;
        }
        ~processpool() {
            delete[]m_sub_process;
        }
        void run(); //启动进程池
};
template<typename T>
processpool<T>* processpool<T>:: m_instance = NULL;

static int sig_pipefd[2]; //处理信号
static int setnonblocking(int fd) {
    int old_op = fcntl(fd, F_GETFL);
    int new_op = old_op | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_op);
    return old_op;
}
static void addfd(int epollfd, int fd) {
    epoll_event eve;
    eve.events = EPOLLIN | EPOLLET;
    eve.data.fd = fd;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &eve);
    setnonblocking(fd);
}

static void removefd(int epollfd, int fd) {
    epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, NULL);
    setnonblocking(fd);
}
static void sig_handler(int sig) {//为了统一事件源
    int save_errno = errno;
    int msg = sig;
    send(sig_pipefd[1], (char*)&msg, 1, 0);
    errno = save_errno;
}
static void addsig(int sig, void(handler)(int), bool restart = true) {
    struct sigaction sa;
    memset(&sa, 0, sizeof sa);
    sa.sa_handler = handler;
    if(restart) 
        sa.sa_flags |= SA_RESTART;
    sigfillset(&sa.sa_mask);
    assert(sigaction(sig,&sa, NULL) != -1);    
}

template<typename T>
processpool<T>:: processpool(int listenfd, int process_number): //构造函数
m_listenfd(listenfd), m_process_number(process_number), m_idx(-1), m_stop(false)
{
    assert(process_number > 0 && process_number <= MAX_PROCESS_NUMBER);
    m_sub_process = new process[process_number];
    assert(m_sub_process);

    for(int i = 0; i < process_number; i++) {
        int ret = socketpair(PF_UNIX, SOCK_STREAM, 0, m_sub_process[i].m_pipefd);
        assert(ret == 0);
        m_sub_process[i].m_pid = fork(); //返回2次，尽在父进程保存
        assert(m_sub_process[i].m_pid >= 0);
        if(m_sub_process[i].m_pid > 0) {
            close(m_sub_process[i].m_pipefd[1]);
            continue;
        }//父进程
        else {
            close(m_sub_process[i].m_pipefd[0]);
            m_idx = i; //第i个线程
            break; //创建完毕
        }
    }
}
template<typename T>
void processpool<T>::setup_sig_pipe() { //事件监听表

    m_epollfd = epoll_create(5);
    assert(m_epollfd != -1);
    int ret = socketpair(PF_UNIX, SOCK_STREAM, 0, sig_pipefd);
    assert(ret != -1);
    setnonblocking(sig_pipefd[1]);
    addfd(m_epollfd, sig_pipefd[0]);
    addsig(SIGCHLD, sig_handler);
    addsig(SIGTERM, sig_handler);
    addsig(SIGINT, sig_handler);
    addsig(SIGPIPE, SIG_IGN); //忽略

} 
template<typename T>
void processpool<T>::run() {
    if(m_idx != -1) {
        run_child();
        return;
    }
    run_parent();
}
template<typename T>
void processpool<T>::run_child() {
    setup_sig_pipe();
    int pipefd = m_sub_process[m_idx].m_pipefd[1];
    addfd(m_epollfd, pipefd);
    epoll_event events[MAX_EVENT_NUMBER];
    T*users = new T[USER_PER_PROCESS];
    assert(users);
    int number = 0;
    while(!m_stop) {
        number = epoll_wait(m_epollfd, events, MAX_EVENT_NUMBER, -1);
        if(number < 0 && errno != EINTR) {
            printf("Epoll failure\n");
            break;
        }
        for(int i = 0; i < number; i++) {
            int sockfd = events[i].data.fd;
            if(sockfd == pipefd && (events[i].events& EPOLLIN)) {
                int client = 0;
                ret = recv(sockfd, (char*)&client, sizeof client, 0);
                if(ret == 0 || (ret < 0 && errno != EAGAIN)) {
                    continue;
                }
                else {
                    struct sockaddr_in client_address;
                    socklen_t client_addrlength = sizeof client_address;
                    
                }
            }
        }
    }
}


#endif