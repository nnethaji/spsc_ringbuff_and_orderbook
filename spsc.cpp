#include <condition_variable>
#include <cstddef>
#include <mutex>
#include <stdio.h>
#include <array>
#include <cstdint>
#include <sys/wait.h>

template <typename T, size_t N>
class SPSCQueue {
    std::array<T, N> ring_buff;
    static_assert((N & (N - 1)) == 0 ,"N should be a power of 2");
    std::mutex m;
    std::condition_variable not_empty; //notify consumer
    std::condition_variable not_full; // notify producer
    size_t head{0};  
    size_t tail{0}; 

public:
    void push(T value){   
        std::unique_lock<std::mutex> lk(m);
        // producer gets lock
        // checks predicate
        // true (means q is not full) then executes and call not_empty
        // false (false means q is full) then gives up lock
        // goes to sleep
        // will wait for not_full.notify_one
        not_full.wait(lk, [&]() {
            return (head - tail < N);
        });
        ring_buff[head&(N-1)] = value;
        head++;
        lk.unlock();
        not_empty.notify_one();
    }

    void pop(T& value){     
        // consumer thread gets the lock
        // only to check predicate
        std::unique_lock<std::mutex> lk(m);
        // predicate is false (means q is empty)
        // C goes to sleep
        // if true (q is not empty)
        // C consumes
        not_empty.wait(lk, [&](){
            return (head != tail);
        });
        // if false the below code blocks and doesnt run
        // nobody holds the lock now  
        // producer acquires lock so executes push code
        // he goest to push() and checks not_full wait that is true and executes and then not_empty.notify_one happens
        // now consumer is woken up, he acquires lock checks again
        // true this time so below code runs        
        // and wakes up producer
        value = ring_buff[tail&(N-1)];
        tail++;
        lk.unlock();
        not_full.notify_one();

    }
};



