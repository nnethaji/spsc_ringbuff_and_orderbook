#include "spsc.hpp"
#include <cstdint>
#include <iostream>
#include <thread>
#include <vector>

int main(){
    SPSCQueue<msg, 8> lock_udaya_buffer;
    std::vector<std::thread> threads;
    
    threads.emplace_back([&](){
        for(int i=0; i<1000; i++){
            msg m{};
            m.sequence_no = i;
            lock_udaya_buffer.push(m);
        }
        lock_udaya_buffer.shutdown();
    });
    
    uint64_t i =0;
    threads.emplace_back([&]() {
        msg seq;
        while(lock_udaya_buffer.pop(seq)){
            if(seq.sequence_no != i){
                std::cerr<< "out of order";
            }
            i++;
            //std::this_thread::sleep_for(std::chrono::microseconds(1));
        }
    });
    

    for(auto& th:threads){
        if(th.joinable()){
            th.join();
        }
    }
    if(i==1000){
        std::cout<<"WORKS";
    }

    return 0;
}
