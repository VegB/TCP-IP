#ifndef RECEIVERTCP_HH
#define RECEIVERTCP_HH
#include <click/element.hh>
#include <click/timer.hh>
#include "packet.hh"

CLICK_DECLS

class ReceiverTCP : public Element {
public:
    ReceiverTCP();
    ~ReceiverTCP();
    const char *class_name() const { return "ReceiverTCP";}
    const char *port_count() const { return "1/1";}
    const char *processing() const { return PUSH; }
    int configure(Vector<String> &conf, ErrorHandler *errh);
    
    void run_timer(Timer*);
    void push(int port, Packet *packet);
    int initialize(ErrorHandler*);
    
private:
    Timer _timerHello;
    Timer _timerTO;
    
    uint32_t _seq;  // counting in this client
    uint32_t _delay;
    uint32_t _period;
    uint32_t _periodHello;
    uint32_t _time_out;
    uint32_t _my_address;
    uint32_t _other_address;
    int transmissions;
    
    // Transmission Control Block
    uint32_t _window_size;
    uint32_t _offset;
    uint8_t _my_state;
    uint8_t _other_state;
    uint32_t _empty_receiver_buffer_size;
    uint8_t _finished_transmission;
	TCP_Packet _duplicate_packet;  // may be the duplicate of SYNACK or FIN
    
    // Generating Packets
    WritablePacket* CreateDataPacket();
    WritablePacket* CreateOtherPacket(packet_types type_of_packet, TCP_Header* header);
    bool ReadDataFromFile(char*);
    bool Valid_ACK(struct TCP_Header* header);
    //Packet* CreatePacket();
};

CLICK_ENDDECLS
#endif

