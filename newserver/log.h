#ifndef LOG_H
#define LOG_H
#include "work.h"
#include "lock.h"
#include <string>
#include <deque>
#include <queue>
#include <ctime>
#include <pthread.h>
using namespace std;
template<class T>
struct log_queue {
    locker m_mutex;
    cond m_cond;
    int m_max_size;
    int m_size;
    queue<T>q;
    bool push(const T&ele) {
        m_mutex.lock();
        if(q.size() > m_max_size) {
            m_cond.broadcast();
            m_mutex.unlock();
            return false;
        }
        q.push(T);
        m_size++;
        m_cond.broadcast();
        m_mutex.unlock();
        return true;
    }
    bool pop(T &item) {
        m_mutex.lock();
        while(m_size <= 0) {
            if(!m_cond_wait(m))
        }
    }

};
void* handle_log(void* ele);

class log : public work {
    public:
    locker lock;
    static log* get_instance () {
        static log lg;
        return &lg;
    }
    
    void init(const char *file_name,int log_buf_size, int max_queue_size) {
        pthread_t tid;
        pthread_create(&tid, NULL,handle_log, this);
    }


};
void* handle_log(void* ele) {
    log* th = (log*)ele;
    th->lock.lock();


}

#endif