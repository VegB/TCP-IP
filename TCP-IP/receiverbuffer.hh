#ifndef ReceiverBuffer_HH
#define ReceiverBuffer_HH
#include <click/element.hh>
#include <click/timer.hh>
#include "packet.hh"

#define RECEIVER_BUFFER_SIZE 2

CLICK_DECLS

class ReceiverBuffer : public Element {
public:
    ReceiverBuffer();
    ~ReceiverBuffer();
    const char *class_name() const { return "ReceiverBuffer";}
    const char *port_count() const { return "2/2";}
    const char *processing() const { return PUSH; }

    void run_timer(Timer*);    
    void push(int port, Packet *packet);
    int initialize(ErrorHandler*);
    
private:
    Timer _timerSend;
    char _receiver_buffer[RECEIVER_BUFFER_SIZE * sizeof(struct TCP_Packet)];
    uint8_t _receiver_start_pos;
    uint8_t _receiver_end_pos;
    uint32_t _expected_seq;  // the seq of last received packet seq + 1
    uint8_t _send_interval;    
    WritablePacket* CreateInfoPacket();
    WritablePacket* ReadOutDataPacket();
    uint8_t ReceiverBufferRemainSize(uint8_t s, uint8_t e);
    bool ReceiverBufferFull();
    bool ReceiverBufferEmpty();
    uint32_t GetFirstSeqInReceiverBuffer();
};

CLICK_ENDDECLS
#endif


