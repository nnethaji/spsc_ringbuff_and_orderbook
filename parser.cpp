#include <cstdint>
#include <chrono>
#include <cstdlib>
#include <fstream>
#include <ios>
#include <bit>
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

void generate(const char* path, int n){

    std::ofstream out(path,std::ios::binary);
    for(int i = 0; i<n ; i++){
        
        msg m;
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


void parse(const char* path){
    std::ifstream in(path, std::ios::binary);
    msg receiver;

    while(in.read( reinterpret_cast<char*>(&receiver), sizeof(receiver))){
        receiver.sequence_no = std::byteswap(receiver.sequence_no);
        receiver.timestamp_ns = std::byteswap(receiver.timestamp_ns);
        receiver.price = std::byteswap(receiver.price);
        receiver.quantity = std::byteswap(receiver.quantity);
        receiver.symbol_id = std::byteswap(receiver.symbol_id);
        
    }

}

