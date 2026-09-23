#include "spsc.hpp"
#include <chrono>
#include <cstdlib>
#include <ios>

void generate(const char* path, int n){

    std::ofstream out(path,std::ios::binary);
    for(int i = 0; i<n ; i++){
        
        msg m{};
        m.sequence_no = i+1;
        m.sequence_no = std::byteswap(m.sequence_no);

        std::chrono::system_clock::time_point arrival_time = std::chrono::system_clock::now();
        m.timestamp_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
            arrival_time.time_since_epoch()
        ).count();
        m.timestamp_ns = std::byteswap(m.timestamp_ns);

        m.price = random();
        m.price = std::byteswap(m.price);

        m.quantity = rand();
        m.quantity = std::byteswap(m.quantity);

        m.symbol_id = rand()%1234;
        m.symbol_id = std::byteswap(m.symbol_id);

        m.buy_sell = (rand()%2 ==0)?'B':'S';
        m.msg_type = rand()%3;
        out.write(reinterpret_cast<const char*>(&m), sizeof(m));

    }
    
}


