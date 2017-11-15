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
    struct TCP_Header header = (struct TCP_Header)packet->header;
	if(header.type == DATA) {
		click_chatter("Received DATA from %u with destination %u", header.source, header.destination);
		int next_port = _ports_table.get(header.destination);
		output(next_port).push(income_packet);
	}
    else if(header.type == HELLO) {
		click_chatter("Received HELLO from %u on port %d", packet->header.source, port);
		_ports_table.set(header.source, port);
	}
    else if(header.type == SYN) {
		click_chatter("Received SYN from %u on port %d", packet->header.source, port);
		_ports_table.set(header.source, port);
                int next_port = _ports_table.get(header.destination);
		output(next_port).push(income_packet);
	}
    else if(header.type == SYNACK) {
                click_chatter("Received SYNACK from %u on port %d", packet->header.source, port);
                _ports_table.set(header.source, port);
                int next_port = _ports_table.get(header.destination);
                output(next_port).push(income_packet);
        }
    else if(header.type == ACK) {
                click_chatter("Received ACK from %u on port %d", packet->header.source, port);
                _ports_table.set(header.source, port);
                int next_port = _ports_table.get(header.destination);
                output(next_port).push(income_packet);
        }
    else if(header.type == FIN) {
                click_chatter("Received FIN from %u on port %d", packet->header.source, port);
                _ports_table.set(header.source, port);
                int next_port = _ports_table.get(header.destination);
                output(next_port).push(income_packet);
        }
    else {
		click_chatter("Wrong packet type: %u", header.type);
		income_packet->kill();
	}
}

CLICK_ENDDECLS 
EXPORT_ELEMENT(BasicRouter)
