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
	if(packet->header.type == 0) {
		output(0).push(income_packet);
	}
    else if(packet->header.type == 1) {
		output(1).push(income_packet);
	}
    else if(packet->header.type == 2) {
		output(2).push(income_packet);
	}
    else {
		click_chatter("Wrong packet type");
		income_packet->kill();
	}
}

CLICK_ENDDECLS 
EXPORT_ELEMENT(BasicIP)
