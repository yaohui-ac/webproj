#ifndef WEBSERVER_H
#define WEBSERVER_H

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <cassert>
#include <sys/epoll.h>

const int MAX_FD = 65536;           //最大文件描述符
const int MAX_EVENT_NUMBER = 10000; //最大事件数
const int TIMESLOT = 5;             //最小超时单位

class webserver {
public:
    webserver();
    ~webserver();
    // 初始化部分
    // 日志库部分
    // 定时器部分
    // 线程池部分
    // 处理http请求部分
    // 数据库连接池部分
    

};
#endif