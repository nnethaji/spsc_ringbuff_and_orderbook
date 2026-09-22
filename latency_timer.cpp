#include <chrono>
class LatencyTimer{
    std::chrono::steady_clock::time_point start;
    std::chrono::steady_clock::time_point end;

public: 
     void start_time(){
                start = std::chrono::steady_clock::now();
     } 
     void end_time(){
               end = std::chrono::steady_clock::now();
     } 
     long long elapsed_ns() {
               long long formatted_time = std::chrono::duration_cast<std::chrono::nanoseconds>(end- start).count(); 
               return formatted_time; 
      }

};