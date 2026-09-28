// delete the mutex and both condition variables
// push returns bool — false if full
// pop returns bool — false if empty

#include <array>
#include <atomic>
#include <chrono>
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

// struct with timestamp for latency measure
struct Latency_measured_buffer {
    msg m; 
    std::chrono::steady_clock::time_point time_latency_stamp;
};


template <typename T, size_t N>
class SPSCQueue {
    std::array<T, N> ring_buff;
    static_assert((N & (N - 1)) == 0 ,"N should be a power of 2");
    
    std::atomic<size_t> head{0};  
    std::atomic<size_t> tail{0}; 
    std::atomic <bool> finished_production{false};  // flag to finally tell the consumer that the producer is done
    // atomic otherwise not just timing miss 
    // but undefined behaviour - compiler might take the bool out of a while loop 

public:
    
    bool push(T value){   
                
        size_t h = head.load(std::memory_order::relaxed);
        size_t t = tail.load(std::memory_order::acquire);
        
        if(!(h-t<N)) {
            return false;
        }
        ring_buff[h&(N-1)] = value;
        // release prevents the store to head from becoming visible before the slot write drains out of the store buffer. 
        // The value must land in shared cache before the head does
        head.store(h+1, std::memory_order::release);
        return true;;
    }

    bool pop(T& value){     
        // auto f_p_bool = finished_production.load(std::memory_order::acquire);
        size_t t = tail.load(std::memory_order::relaxed);
        size_t h = head.load(std::memory_order::acquire);
        
        //the empty test has to happen regardless of the flag
        // The flag then answers
        // was that empty temp or final?

        // below check is dangerous cause f_p_bool might be false 
        // because production might be done but the f_p_bool is not set yet 
        // if you skip the loop // you will end up reading already read queue element again
        // and you will advance tail in an empty queue 
        // tail > head means h - t underflows to a gigantic number,
        // so the producer's fullness check h - t < N is false forever

        // if(f_p_bool){
        //     if(h==t){
        //         return false;
        //     }
        // }

        // value = ring_buff[t&(N-1)];
        // tail.store(t+1, std::memory_order::release); 
        // return true;

        // this is better
        if (h != t) { 
            value = ring_buff[t&(N-1)];
            tail.store(t+1, std::memory_order::release); 
            return true; 
        }
        return false;
        // return !f_p_bool;        // empty — but only stop if the flag was already set
        // return !f_p_bool (instead of h==t return false else consume),  wrong cause you wouldn't be able to return false
        // even if production is not finished but h==t right now 
        // if done is false, return true so is not fin and will try again
        // you might be wrong for one extra loop, which is ok — you just loop and re-check.
        

        //if we do head == tail check first and it is true and  
        // then again producer  pushs items and calls shutdown 
        // and then you check production done you will lose the items

        // but slo ifthe producer pushes 10 items, then shutdown the consumer is lagging behind and the queue still holds 3
        // If the consumer checks finished_prod first the items get lost
        // but in this latter case head == tail will not be true so we shouldn't put them in an AND condition 
        // we should check both separately // and not in a nested if //we should check both independently 
        // first production_done check and then check head==tail else spin until production_done is set

        // so h==t will not be true now => if(h!=t) consume elements otherwise 
        // if h==t true then pop will return true but since done is false it will keep spinning
    }
    bool done() const {
            return finished_production.load(std::memory_order::acquire) && 
                head.load(std::memory_order::acquire) == tail.load(std::memory_order::relaxed); //cons than check panran
                // tail avan udayathu than
    }
    void shutdown(){
        { 
            finished_production.store(true, std::memory_order::release);
        }
        // put it in scope so that lock unlocks before notifying 
    }
};

template <size_t  N>
void parse(const char* path, SPSCQueue <Latency_measured_buffer, N>& spscq){
    std::ifstream in(path, std::ios::binary);
    // msg receiver;
    Latency_measured_buffer receiver_and_stamp;  
    msg& receiver = receiver_and_stamp.m;
    while(in.read( reinterpret_cast<char*>(&receiver), sizeof(receiver))){
        receiver.sequence_no = std::byteswap(receiver.sequence_no);
        receiver.timestamp_ns = std::byteswap(receiver.timestamp_ns);
        receiver.price = std::byteswap(receiver.price);
        receiver.quantity = std::byteswap(receiver.quantity);
        receiver.symbol_id = std::byteswap(receiver.symbol_id);
        

        // make stamp before you push
        receiver_and_stamp.time_latency_stamp = std::chrono::steady_clock::now();
        //produces 
        //spin if buffer is full and consumer hasnt consumed yet 
        while(!spscq.push(receiver_and_stamp)){
                    // is essentially waiting for consumer to consume

        }
        
    }
   
    // producer calls shutdown
    spscq.shutdown();
    
}

void generate(const char* path, int n);