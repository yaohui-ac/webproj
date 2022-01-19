#ifndef TIME_WHEEL_TIMER
#define TIME_WHELL_TIMER
#include<time.h>
#include<netinet/in.h>
#include<stdio.h>

#define BUFFER_SIZE 64
class tw_timer;
class client_data {
    public:
        sockaddr_in address;
        int sockfd;
        char buf[BUFFER_SIZE];
        tw_timer*timer;
};

class tw_timer {
    public:

    tw_timer(int rot, int ts):
    next(NULL), prev(NULL), rotation(rot), time_slot(ts) {

    }
    int rotation;
    int time_slot;
    client_data*user_data; 
    void (*cb_func)(client_data*);
    tw_timer *next;
    tw_timer *prev;
};
class time_whell {
    public:
    time_whell():
    cur_slot(0) {
        for(int i = 0; i < N; i++) slots[i] = NULL;
    }
    ~time_whell() { //
        for(int i = 0; i < N; i++) {
            tw_timer* tmp = slots[i];
            while(tmp) {

                slots[i] = tmp ->next;
                delete tmp;
                tmp = slots[i];

            }
        }
    }
    tw_timer* add_timer (int timeout, tw_timer* timer) { 
        // 根据定时值插入槽中,传入的值应为相对于当前的时间槽 
        // 需要传入与用户信息有关东西，所以需要传入tw_timer指针,到时将执行对应的操作
        // timeout传入，最终事件放在事件轮上是相对时间，相对于当前时间，在其后多少圈，第几槽
        // 如果timeout是绝对时间，则需要与当前时间做差。因此timeout是时间差-->(即延时多少后判断)
        if(timeout < 0) return NULL;
        int ticks = 0;
        if(timeout < SI) {
            ticks = 1;
        }
        else {
            ticks = timeout / SI;
        }
        int rotation = ticks / N;
        int ts = (cur_slot + (ticks % N) )%N;
        timer->rotation = rotation;
        timer->time_slot = ts;
          //转动多少圈被触发, 以及位于的槽
        if(!slots[ts]) {
            printf("Add timer rotation is %d, ts is %d，cur_slot is %d\n", rotation, ts, cur_slot);
            slots[ts] = timer;
        }else {
            timer->next = slots[ts];
            slots[ts]->prev = timer;
            slots[ts] = timer;
        }
        return timer;
    }
    void del_timer(tw_timer * timer) {
        if(!timer) {
            return;
        }
        int ts = timer->time_slot;
        if(timer == slots[ts]) {
            slots[ts] = slots[ts]->next;
            if(slots[ts])
                slots[ts]->prev = NULL;
            delete timer;    
        }else {
            timer->prev->next = timer->next;
            if(timer->next)
                timer->next->prev = timer->prev;
            delete timer;    
        }
    }
    void tick() {
        tw_timer* tmp = slots[cur_slot]; //取得当前时间轮当前槽头节点
        printf("Current slot is %d\n", cur_slot);
        while(tmp) {
            printf("Tick the timer once \n");
            if(tmp ->rotation > 0) {
                tmp->rotation--;
                tmp = tmp->next; //未到期
            }
            else {
                tmp->cb_func(tmp->user_data); //定时器到期
                if(tmp == slots[cur_slot]) {
                    printf("delete header in cur_slot\n");
                    slots[cur_slot] = tmp->next;
                    delete tmp;
                    if(slots[cur_slot])
                        slots[cur_slot]->prev = NULL;
                    tmp = slots[cur_slot];
                }else {
                    tmp->prev->next = tmp->next;
                    if(tmp->next) {
                        tmp->next->prev = tmp->prev;
                    }
                    tw_timer* tmp2 = tmp ->next;
                    delete tmp;
                    tmp = tmp2;
                }
                
            }
        }
        cur_slot = ++cur_slot % N;
    }

    private:
    static const int N = 60;
    static const int SI = 1; //心跳时间
    tw_timer * slots[N]; //时间轮的槽
    int cur_slot; //当前槽
};

#endif