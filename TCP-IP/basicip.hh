#ifndef CLICK_BASICCLASSIFIER_HH 
#define CLICK_PACKETGENERATOR_HH 
#include <click/element.hh>
#include <click/timer.hh>

CLICK_DECLS

class BasicIP : public Element {
    public:
        BasicIP();
        ~BasicIP();
        const char *class_name() const { return "BasicIP";}
        const char *port_count() const { return "4/4";}
        const char *processing() const { return PUSH; }
		
		void push(int port, Packet *packet);
        int initialize(ErrorHandler*);
        WritablePacket* add_IP_header(Packet *income_packet);
        WritablePacket* tear_down_IP_header(Packet *income_packet);
}; 

CLICK_ENDDECLS
#endif 
