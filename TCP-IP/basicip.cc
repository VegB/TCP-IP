#include <click/config.h>
#include <click/confparse.hh>
#include <click/error.hh>
#include <click/timer.hh>
#include <click/packet.hh>
#include "basicip.hh" 
#include "packets.hh"
#include "packet.hh"

CLICK_DECLS 

BasicIP::BasicIP() {
    click_chatter("Creating a BasicIP object.");
}

BasicIP::~BasicIP(){
    click_chatter("Killing a BasicIP object.");
}

int BasicIP::initialize(ErrorHandler *errh){
    return 0;
}

void BasicIP::push(int port, Packet *income_packet) {
	assert(income_packet);
	struct TCP_Packet *packet = (struct TCP_Packet *)income_packet->data();
    struct TCP_Header header = (struct TCP_Header)packet->header;
	/*if(header.type == 0) {
		output(0).push(income_packet);
	}
    else if(header.type == 1) {
		output(1).push(income_packet);
	}
    else if(header.type == 2) {
		output(2).push(income_packet);
	}
    else {
		click_chatter("Wrong packet type in IP: %u", header.type);
	//	income_packet->kill();
		output(0).push(income_packet);
	}*/
	output(0).push(income_packet);
}

CLICK_ENDDECLS 
EXPORT_ELEMENT(BasicIP)
