#include <array>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <mutex>
#include <stdio.h>
#include <sys/wait.h>
#include <bit>
#include <fstream>


// offset	      field	         type	     size
// 0	        sequence_no 	 uint64_t	   8
// 8	        timestamp_ns	 uint64_t	   8
// 16	           price	     int64_t	   8
// 24	          quantity	     uint32_t	   4
// 28	           SYMBOL 	     uint16_t	   2
// 30	           buy/sell	     char	       1
// 31	           msg_type	     uint8_t	   1


struct msg {
    uint64_t sequence_no;
    uint64_t timestamp_ns;
    int64_t price;
    uint32_t quantity;
    uint16_t symbol_id;
    char buy_sell;
    uint8_t msg_type;
};

static_assert(sizeof(msg) == 32, "msg is not 32 bytes");




template <typename T, size_t N>
class SPSCQueue {
    std::array<T, N> ring_buff;
    static_assert((N & (N - 1)) == 0 ,"N should be a power of 2");
    std::mutex m;
    std::condition_variable not_empty; //notify consumer
    std::condition_variable not_full; // notify producer
    size_t head{0};  
    size_t tail{0}; 
    bool finished_production{false};  // flag to finally tell the consumer that the producer is done



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

    bool pop(T& value){     
        // consumer thread gets the lock
        // only to check predicate
        std::unique_lock<std::mutex> lk(m);
        // predicate is false (means q is empty)
        // C goes to sleep
        // if true (q is not empty)
        // C consumes
        not_empty.wait(lk, [&](){
            return (head != tail || finished_production);
        });
        // if false the below code blocks and doesnt run
        // nobody holds the lock now  
        // producer acquires lock so executes push code
        // he goest to push() and checks not_full wait that is true and executes and then not_empty.notify_one happens
        // now consumer is woken up, he acquires lock checks again
        // true this time so below code runs        
        // and wakes up producer
        if(head != tail){
            value = ring_buff[tail&(N-1)];
            tail++;
            lk.unlock();
            not_full.notify_one();
            return true;
        }
        
        return false;
        

    }

    void shutdown(){
        { 
            std::lock_guard lk(m); 
            finished_production = true; 
        }
        // put it in scope so that lock unlocks before notifying 
        not_empty.notify_all();
    }
};

template <size_t  N>
void parse(const char* path, SPSCQueue <msg, N>& spscq){
    std::ifstream in(path, std::ios::binary);
    msg receiver;

    while(in.read( reinterpret_cast<char*>(&receiver), sizeof(receiver))){
        receiver.sequence_no = std::byteswap(receiver.sequence_no);
        receiver.timestamp_ns = std::byteswap(receiver.timestamp_ns);
        receiver.price = std::byteswap(receiver.price);
        receiver.quantity = std::byteswap(receiver.quantity);
        receiver.symbol_id = std::byteswap(receiver.symbol_id);
        
        //produces
        spscq.push(receiver);
        
    }
   
    // producer calls shutdown
    spscq.shutdown();
    
}

void generate(const char* path, int n);