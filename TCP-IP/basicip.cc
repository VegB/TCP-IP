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

WritablePacket* ReceiverBuffer::CreateAckPacket(TCP_Header* header){
    WritablePacket *packet = Packet::make(0, 0, sizeof(struct TCP_Header), 0);
    struct TCP_Packet* packet_ptr = (struct TCP_Packet*)packet->data();
    struct TCP_Header* header_ptr = (struct TCP_Header*)(&(packet_ptr->header));
    
    memset(packet_ptr, 0, packet->length());
    click_chatter("[ReceiverBuffer]: Send ACK for DATA packet %u", header->sequence);
    
    /* Write TCP_Header */
    header_ptr->source = header->destination;
    header_ptr->destination = header->source;
    header_ptr->empty_buffer_size = ReceiverBufferRemainSize(_receiver_start_pos, _receiver_end_pos);
    
    /* choose ACK type */
    if(header->type == DATA){
        header_ptr->type = ACK;
    }
    else if(header->type == SYN){
        header_ptr->type = SYNACK;
    }
    else if(header->type == FIN){
        header_ptr->type = FINACK;
    }
    
    /* set ack number */
    header_ptr->ack = _last_acked;
    return packet;
}

WritablePacket* BasicIP::add_IP_header(Packet *income_packet){
    struct TCP_Packet* income_packet_ptr = (struct TCP_Packet*)income_packet->data();
    struct TCP_Header* income_packet_header = (struct TCP_Header*)(&(income_packet_ptr->header));
    
    WritablePacket *packet = Packet::make(0, 0, sizeof(struct IP_Packet), 0);
    struct IP_Packet* packet_ptr = (struct IP_Packet*)packet->data();
    struct IP_Header* header_ptr = (struct IP_Header*)(&(packet_ptr->header));
    
    int TCP_Packet_size =sizeof(struct TCP_Packet);
    memcpy((void*)(packet_ptr->data), (const void*)income_packet_ptr, TCP_Packet_size);
    
    /* fill in IP Header */
    header_ptr->type = income_packet_header->type;
    header_ptr->sequence = income_packet_header->sequence;
    header_ptr->source = income_packet_header->source;
    header_ptr->destination = income_packet_header->destination;
    header_ptr->ECN = 0;
}

WritablePacket* BasicIP::tear_down_IP_header(Packet *income_packet){
    struct IP_Packet *income_packet_ptr = (struct IP_Packet*)income_packet->data();
    
    WritablePacket *packet = Packet::make(0, 0, sizeof(struct TCP_Packet), 0);
    struct TCP_Packet* packet_ptr = (struct TCP_Packet*)packet->data();
    
    int TCP_Packet_size =sizeof(struct TCP_Packet);
    memcpy((void*)packet_ptr, (const void*)(income_packet_ptr->data), TCP_Packet_size);
    return packet;
}
CLICK_ENDDECLS
EXPORT_ELEMENT(BasicIP)
