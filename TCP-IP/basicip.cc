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
    click_chatter("[BasicIP]: Creating a BasicIP object.");
}

BasicIP::~BasicIP(){
    click_chatter("[BasicIP]: Killing a BasicIP object.");
}

int BasicIP::initialize(ErrorHandler *errh){
    return 0;
}

void BasicIP::push(int port, Packet *income_packet) {
	assert(income_packet);
	struct TCP_Packet *packet = (struct TCP_Packet *)income_packet->data();
	struct TCP_Header header = (struct TCP_Header)packet->header;
	if(port == 0){ // from SenderBuffer
		output(1).push(income_packet);
		// add IP header
	}
	else if(port == 1){ // from router to sender
		output(0).push(income_packet); 
		// Wrip IP header
	}
	else if(port == 2){ // from router to receiver
		output(3).push(income_packet);
		// Wrip IP header
	}
	else if(port == 3){ // from Receiver buffer
    		output(2).push(income_packet);
		// add IP header
	}
}

CLICK_ENDDECLS 
EXPORT_ELEMENT(BasicIP)
