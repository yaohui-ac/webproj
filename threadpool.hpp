#ifndef THREADPOOL_HPP
#define THREADPOOL_HPP
#include<list>
#include<cstdio>
#include<exception>
#include<pthread.h>
#include"lock.hpp"
template<typename T>
class threadpool {
    private:
        static void* worker(void* arg);//工作线程
        void run();
    private:
        int m_thread_number; // 线程池中线程数
        int m_max_requests; // 请求队列中允许的最大请求数
        pthread_t* m_threads; // 描述线程池的数组
        std::list<T*>m_workqueue; // 请求队列
        locker m_queuelocker; // 请求队列的互斥锁
        sem m_queuestat; // 是否有任务需要处理
        bool m_stop;    //是否结束线程
    public:
        threadpool(int thread_number = 8, int max_requests = 10000);
        ~threadpool();
        bool append(T*request); //添加任务
};

template<typename T>
threadpool<T>::threadpool(int thread_number, int max_requests):
m_thread_number(thread_number), m_max_requests(max_requests),
m_stop(false), m_threads(NULL)
{
    if(thread_number <= 0 || max_requests <= 0)
        throw std::exception();
    m_threads = new pthread_t[m_thread_number];
    if(!m_threads)
        throw std::exception();
    for(int i = 0; i < thread_number; i++) {
        printf("Create the %dth thread\n", i);
        if(pthread_create(m_threads + i, NULL, worker, this)) {
            delete []m_threads;
            throw std::exception();
        }
        if(pthread_detach(m_threads[i])) {
            delete []m_threads;
            throw std::exception();
        }
    } 
}

template<typename T>
threadpool<T>::~threadpool() {
    delete []m_threads;
    m_stop = true;
}

template<typename T>
bool threadpool<T>::append(T* request) {
    m_queuelocker.lock(); //线程去竞争锁
    if(m_workqueue.size() > m_max_requests) {
        m_queuelocker.unlock();
        return false;
    }
    m_workqueue.push_back(request);
    m_queuelocker.unlock();
    m_queuestat.post();
    return true;
}

template<typename T>
void* threadpool<T>::worker(void* arg) {
    threadpool* pool = static_cast<threadpool*>arg;
    pool->run();
    return pool; 
}
template<typename T>
void threadpool<T>::run() {
    while(!m_stop) {
        m_queuestat.wait(); //工作线程阻塞至此
        m_queuelocker.lock();

        if(m_workqueue.empty()) {
            m_queuelocker.unlock();
            continue;
        }
        T* request = m_workqueue.front();
        m_workqueue.pop_front();
        m_queuelocker.unlock();
        if(!request) 
            continue;
        request->process(); //T 类型需要实现 process    

    }
}


#endif