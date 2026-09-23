Implementation of a UDP byte stream parser -> writes to a SPSC ring buffer (uses mutex and cv) -> order book

next imp: will use lock free queue 
