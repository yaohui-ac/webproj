#ifndef HI_REDIS_HPP
#define HI_REDIS_HPP
#include <iostream>
#include <string.h>
#include <string>
#include <stdio.h>
#include"lock.hpp"
#include <hiredis/hiredis.h>
#include<stdarg.h>
using namespace std;
class Redis {
    private:
        redisContext* connect;
        redisReply* reply;
        int returnType() {
            int ret = reply->type;
            freeReplyObject(this->reply);
            return ret;
        }
        long long returnInteger() {
            long long ret = reply->integer;
            freeReplyObject(this->reply);
            return ret;
        }    
        size_t returnLen() {
            size_t ret = reply->len;
            freeReplyObject(this->reply);
            return ret;
        }
        std::string returnStr() {
            std::string ret(reply->str);
            freeReplyObject(this->reply);
            return ret;
        }
        bool returnStatus() {
            bool ret = 0;
            if(reply->str[0] == 'O')
                ret = 1;
            freeReplyObject(this->reply);
            return ret;
        }
        Redis(){}

        ~Redis() {
            this->reply = NULL;	    	    
        }
    public:
        static Redis* onlyone; 
        static locker mylock;
         Redis* static createRedisConnection() {
            // 单例模式
            mylock.lock();
            if(onlyone == nullptr) {
                onlyone = new Redis();
                onlyone->init_redis("127.0.0.1", 3389);
            }
            mylock.unlock();
            return onlyone;
        }
        bool init_redis(std::string host, int port) {
            timeval timeout = { 5, 500000 };
            this->connect = redisConnectWithTimeout(host.c_str(), port, timeout);
            if(this->connect != NULL && this->connect->err) {
                printf("connect error: %s\n", this->connect->errstr);
                return 0;
            }
            return 1;
        }

        std::string get(std::string key) {
            this->reply = (redisReply*)redisCommand(this->connect, "GET %s", key.c_str());
            return returnStr();
        }

        bool set(std::string key, std::string value) {
           reply = (redisReply*)redisCommand(this->connect, "SET %s %s", key.c_str(), value.c_str());
           return returnStatus();

        }
        long long strlen(std::string key) {
            reply = (redisReply*)redisCommand(this->connect, "strlen %s", key.c_str());
            return returnInteger();
        }
        bool append(std::string key, std::string value) {
            reply = (redisReply*)redisCommand(this->connect, "append %s %s", key.c_str(), value.c_str());
            return returnStatus();
        }
        long long incr(std::string key) {
            reply = (redisReply*)redisCommand(this->connect, "incr %s", key.c_str());
            return returnInteger();
        }
        long long incrby(std::string key, long long incrment) {
            reply = (redisReply*)redisCommand(this->connect, "incrby %s %lld", key.c_str(), incrment);
            return returnInteger();
        }
        long long decr(std::string key) {
            reply = (redisReply*)redisCommand(this->connect, "decr %s", key.c_str());
            return returnInteger();
        }
        long long decrby(std::string key, long long incrment) {
            reply = (redisReply*)redisCommand(this->connect, "decrby %s %lld", key.c_str(), incrment);
            return returnInteger();
        }
        std::string incrbyfloat(std::string key,double incrment) {
            reply = (redisReply*)redisCommand(this->connect, "incrbyfloat %s %f", key.c_str(), incrment);
            return returnStr();
        } 
        bool del(std::string key) {
            reply = (redisReply*)redisCommand(this->connect, "del %s", key.c_str());
            return returnStatus();
        }                                                                                    
        bool exists(std::string key) {
            reply = (redisReply*)redisCommand(this->connect, "exists %s", key.c_str());
            return returnInteger();
        }
        /*   hash  ↓ */
        bool hset(std::string key, std::string field, std::string value) {
            reply = (redisReply*)redisCommand(this->connect, "hset %s %s %s", key.c_str(), field.c_str(), value.c_str());
            return returnInteger();
        }
        std::string hget(std::string key, std::string field) {
            reply = (redisReply*)redisCommand(this->connect, "hget %s %s", key.c_str(), field.c_str());
            return returnStr();
        }
        /*持久化 ↓*/
        bool save() {
            reply = (redisReply*)redisCommand(this->connect, "save");
            return returnStatus();

        }
                
};
#endif

