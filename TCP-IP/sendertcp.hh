#ifndef SENDERTCP_HH
#define SENDERTCP_HH 
#include <click/element.hh>
#include <click/timer.hh>
#include "packet.hh"

CLICK_DECLS
#define DATA_PACKET_CNT 500
/* Connection Type */
typedef enum {
    CLOSED = 0,
    CONNECTED,
    FIN_WAIT
} connection_types;

class SenderTCP : public Element {
    public:
        SenderTCP();
        ~SenderTCP();
        const char *class_name() const { return "SenderTCP";}
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
        uint32_t _hello_seq;
		uint32_t _delay;
		uint32_t _period;
		uint32_t _periodHello;
		uint32_t _time_out;
		uint32_t _my_address;
		uint32_t _other_address;
		int transmissions;
    
        /* Transmission Control Block */
        uint32_t _window_size;
        uint32_t _offset;
        uint8_t _my_state;
        uint8_t _other_state;
        uint32_t _empty_sender_buffer_size;
        uint32_t _empty_receiver_buffer_size;
        uint8_t _finished_transmission;
        uint32_t _data_piece_cnt;
        uint8_t _increase_policy;
        uint32_t _slow_start_limit;
        uint32_t _additive_increase_limit;
        uint32_t _ecn_limit;
    
        /* Generating Packets  */
        void CreateDataPacket();
        WritablePacket* CreateOtherPacket(packet_types type_of_packet, TCP_Header* header);
        bool ReadDataFromFile();
        bool Valid_ACK(struct TCP_Header* header);
        bool NeedRetransmission();
};

CLICK_ENDDECLS
#endif 
