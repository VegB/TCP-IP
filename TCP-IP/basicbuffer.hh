#ifndef BasicBuffer_HH
#define BasicBuffer_HH
#include <click/element.hh>
#include <click/timer.hh>
#include "packet.hh"

#define RECEIVER_BUFFER_SIZE 20
#define SENDER_BUFFER_SIZE 20

CLICK_DECLS

class BasicBuffer : public Element {
public:
    BasicBuffer();
    ~BasicBuffer();
    const char *class_name() const { return "BasicBuffer";}
    const char *port_count() const { return "2/2";}
    const char *processing() const { return PUSH; }
    
    void push(int port, Packet *packet);
    int initialize(ErrorHandler*);

private:
    Timer _timerSend;
    char _sender_buffer[SENDER_BUFFER_SIZE * sizeof(struct TCP_Packet)];
    char _receiver_buffer[RECEIVER_BUFFER_SIZE * sizeof(struct TCP_Packet)];
    uint8_t _sender_start_pos;
    uint8_t _sender_end_pos;
    uint8_t _receiver_start_pos;
    uint8_t _receiver_end_pos;
    uint32_t _last_ack;
    uint8_t _send_interval;
    
    WritablePacket* BasicBuffer::CreateInfoPacket();
    uint8_t EmptyBufferSize(uint8_t s, uint8_t e);
};

CLICK_ENDDECLS
#endif

