#ifndef SenderBuffer_HH
#define SenderBuffer_HH
#include <click/element.hh>
#include <click/timer.hh>
#include "packet.hh"

#define SENDER_BUFFER_SIZE 10
#define FAST_RETRANSMIT_BOUND 2

CLICK_DECLS

class SenderBuffer : public Element {
public:
    SenderBuffer();
    ~SenderBuffer();
    const char *class_name() const { return "SenderBuffer";}
    const char *port_count() const { return "2/2";}
    const char *processing() const { return PUSH; }
    
    void push(int port, Packet *packet);
    int initialize(ErrorHandler*);

private:
    char _sender_buffer[SENDER_BUFFER_SIZE * sizeof(struct TCP_Packet)];
    uint8_t _sender_start_pos;
    uint8_t _sender_end_pos;
    uint32_t _last_acked;  // max ACK
    uint8_t _last_acked_cnt;  // how many times '_last_acked' has been received
    
    WritablePacket* ReadOutDataPacket(int);
    WritablePacket* CreateInfoPacket();
    uint8_t SenderBufferRemainSize(uint8_t s, uint8_t e);
    bool SenderBufferFull();
    bool SenderBufferEmpty();
    uint32_t GetSeqInSenderBuffer(int);
    void Retransmit(uint32_t);
    void RetransmitAll();
};

CLICK_ENDDECLS
#endif

