#include <click/config.h>
#include <click/confparse.hh>
#include <click/error.hh>
#include <click/timer.hh>
#include <click/packet.hh>
#include "basicrouter.hh" 
#include "packets.hh"
#include "packet.hh"

CLICK_DECLS 

BasicRouter::BasicRouter() {
    click_chatter("Creating a BasicRouter object.");
}

BasicRouter::~BasicRouter(){
	click_chatter("Killing a BasicRouter object.");
}

int BasicRouter::initialize(ErrorHandler *errh){
    return 0;
}

void BasicRouter::push(int port, Packet *income_packet) {
	assert(income_packet);
    struct TCP_Packet *packet = (struct TCP_Packet *)income_packet->data();
	if(packet->header.type == DATA || packet->header.type == ACK) {
		click_chatter("Received Data from %u with destination %u", packet->header.source, packet->header.destination);
		int next_port = _ports_table.get(packet->header.destination);
		output(next_port).push(income_packet);
	}
    else if(packet->header.type == HELLO) {
		click_chatter("Received Hello from %u on port %d", packet->header.source, port);
		_ports_table.set(packet->header.source, port);
	}
    else {
		click_chatter("Wrong packet type");
		income_packet->kill();
	}
}

CLICK_ENDDECLS 
EXPORT_ELEMENT(BasicRouter)
