#ifndef CERTIFY_HPP
#define CERTIFY_HPP
#include"tools.hpp"
#include<string>
#include<cstring>
#include<unistd.h>
#include<sys/types.h>
#include<fcntl.h>
#include<assert.h>
namespace certification {
    int connfd; //连接套接字
    char buf[1024];
    char* welcome[4] ={ 
        "welcome to tiny server\n",
        "可以选择注册or登录\n",
        "注册命令: register [username] [password]\n",
        "登录命令: signin [username] [password]\n"
    };
    void sendMessage(char* str) {
        int len = strlen(str),sd;
        int haveSend = 0;
        while(haveSend < len) {
            sd = write(connfd, str + haveSend, len - haveSend);
            assert(sd != -1);
            haveSend += sd;
        }
    }
    void sendWelcome() {
        for(int i = 0; i < 4; i++) {
            sendMessage(welcome[i]);
        }
    }
    void certification(int fd) {
        sendWelcome();
        //Redis re0  = Redis();
        //re0.init_redis("127.0.0.1", 3389);

    }
    
};
#endif