#include "spsc_lockfree.hpp"
#include <atomic>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <thread>
#include <vector>
#include <algorithm>

int main(){
    // std::vector<std::thread> threads;
    uint64_t count =0;
    SPSCQueue<Latency_measured_buffer, 1024> lock_buffer;
    
    int n = 10000;
    //generate dummy msg
    generate("trialfile.txt", n);

    std::atomic<bool> consumer_started{false}; 
    //parser calls producer
  
    std::thread producer([&]{ 
        while(!consumer_started.load(std::memory_order_acquire)){

        }
        parse("trialfile.txt", lock_buffer); 
        
    });
    // with optimization consumer becomes faster (mostly templated code) vs producer (a lot of precompiled ifstream)
    // so we want a start check to make both start at same time
    // otherwise producer starts first, produces, and waits for consumer

    std::vector<std::chrono::steady_clock::duration> vector_of_time_durations;
    vector_of_time_durations.reserve(n);

    std::thread consumer([&]{ 
        Latency_measured_buffer msg_and_timestamp{};         
        // while (lock_buffer.pop(m)) 
        //     ++count; 
        consumer_started.store(true,std::memory_order_release);
        while (!lock_buffer.done()) {
            if (lock_buffer.pop(msg_and_timestamp)) {
                vector_of_time_durations.emplace_back(std::chrono::steady_clock::now() - msg_and_timestamp.time_latency_stamp);
                count++;
            }
        
        }
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
    std::sort(vector_of_time_durations.begin(), vector_of_time_durations.end());
    std::cout<< "50p " << vector_of_time_durations[n/2] << "\n";
    std::cout <<"99p "<< vector_of_time_durations[(n*99) / 100] << "\n";
    std::cout <<"99.9p "<<vector_of_time_durations[(n*999) / 1000] << "\n";


    return 0;
}

