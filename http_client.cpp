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
char* get_mess[5] = {
    "GET http://service/settings.txt HTTP/1.1\r\n",
    "Host: www.rt-thread.com\r\n",
    "Connection: keep-alive\r\n",
    "Content-Length: 0\r\n",
    "\r\n"
};
char buf[1024];
int main (int argc, char* argv[]) {
    if(argc <= 2) {
        puts("paremeter error");
        return 1;
    }
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    const char* ip = argv[1];
    int port = atoi(argv[2]);
    assert(fd >= 0);
    sockaddr_in address;
    bzero(&address, sizeof address);
    address.sin_port = htons(port);
    address.sin_family = AF_INET;
    inet_pton(AF_INET, ip , &address.sin_addr);
    socklen_t addrlen = sizeof address;
    int ret = connect(fd, (sockaddr*)&address, addrlen);
    assert(ret != -1);
   // puts("yyy");
    for(int i = 0; i < 5; i++) {
        puts("hhhc1");
        int len = write(fd, get_mess[i], strlen(get_mess[i]));
        std:: cout << len << "\n";
        if(len <= 0) 
            puts("write error");
    }
    while(true) {
        puts("hhhc");
        memset(buf, '\0', sizeof buf);
        int len = read(fd, buf, sizeof buf);
        printf("%s\n", buf);
    }
    return 0;
}