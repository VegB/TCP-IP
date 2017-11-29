#ifndef packets_h
#define packets_h

#include <string>
#define TCP_MAX_LEN 1024
#define FAST_RETRANSMIT 3

// Packet Type
typedef enum {
    DATA = 0,
    ACK,
    SYN,
    SYNACK,
    FIN,
    FINACK,
    HELLO,
    INFO,
    RETRANS
} packet_types;

string packet_name[] = {"DATA", "ACK", "SYN", "SYNACK", "FIN", "FINACK", "HELLO",
    "INFO", "RETRANS"};

// TCP Packet
struct TCP_Header{
    uint8_t type;
    uint8_t source;
    uint8_t destination;
    uint32_t sequence;  // counting in this TCP client!
    uint32_t ack; // which packet it has received
    uint32_t offset;  // offset in current file
    uint32_t size;  // how many bytes of data this tcp packet contains
    uint8_t more_packets;  // is this the end of file?
    uint32_t empty_buffer_size;  // number of packets
};

struct TCP_Packet{
    TCP_Header header;
    char data[TCP_MAX_LEN];
};

// IP Packet
const int IP_DATA_LEN = sizeof(struct TCP_Packet);

struct IP_Header{
    uint8_t type;
    uint8_t sequence;
    uint8_t source;
    uint8_t destination;
    uint32_t size;
    uint8_t ttl;
};

struct IP_Packet{
    IP_Header header;
    char data[IP_DATA_LEN];
};
#endif /* packets_h */
