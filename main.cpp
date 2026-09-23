#include "spsc.hpp"
#include <cstdint>
#include <iostream>
#include <thread>

int main(){
    // std::vector<std::thread> threads;
    uint64_t count =0;
    SPSCQueue<msg, 1024> lock_buffer;

    //generate dummy msg
    generate("trialfile.txt", 10000);

    //parser calls producer
    std::thread producer([&]{ 
        parse("trialfile.txt", lock_buffer); 
    });

    std::thread consumer([&]{ 
        msg m; 
        while (lock_buffer.pop(m)) 
            ++count; 
    });
    
    // threads.emplace_back([&]() {
    //     msg seq;
    //     while(lock_buffer.pop(seq)){
    //         i++;
    //         //std::this_thread::sleep_for(std::chrono::microseconds(1));
    //     }
    // });


    // for(auto& th:threads){
    //     if(th.joinable()){
    //         th.join();
    //     }
    // }
    producer.join();
    consumer.join();
    std::cout << (count == 10000 ? "WORKS" : "FAILED") << " count=" << count << "\n";


    return 0;
}

