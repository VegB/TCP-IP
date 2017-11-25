/*
 1）这边在发FIN SYNACK的时候，如果没有收到ack，就会重发。
     设置timer咯！只检查这两种类型。
 2）设置一个timer检查一下是不是发fin？
 3）我觉得需要设置一个packet大小的buffer，装可能会重发的东西，比如SYNACK, FIN
 */

#include <click/config.h>
#include <click/confparse.hh>
#include <click/error.hh>
#include <click/timer.hh>
#include <click/packet.hh>
#include "receivertcp.hh"
#include "packets.hh"
#include "packet.hh"
#include "receiverbuffer.hh"
#include "sendertcp.hh"

CLICK_DECLS

ReceiverTCP::ReceiverTCP() : _timerTO(this), _timerHello(this) {
        click_chatter("[ReceiverTCP]: Creating a ReceiverTCP object.");
    _seq = 0;
    _period = 3;
    _periodHello = 2;
    _delay = 0;
    _time_out = 1;
    _my_address = 0;
    _other_address = 0;
    transmissions = 0;

	_offset = 0;
    _my_state = CLOSED;
    _other_state = CLOSED;
	_empty_receiver_buffer_size = RECEIVER_BUFFER_SIZE - 1;
	_finished_transmission = 0;
}

ReceiverTCP::~ReceiverTCP(){
        click_chatter("[ReceiverTCP]: Killing a ReceiverTCP object.");
}

int ReceiverTCP::initialize(ErrorHandler *errh){
    _timerTO.initialize(this);
    _timerHello.initialize(this);
    _timerHello.schedule_after_sec(_periodHello);
    return 0;
}

int ReceiverTCP::configure(Vector<String> &conf, ErrorHandler *errh) {
    if (cp_va_kparse(conf, this, errh,
                     "MY_ADDRESS", cpkP+cpkM, cpUnsigned, &_my_address,
                     "OTHER_ADDRESS", cpkP+cpkM, cpUnsigned, &_other_address,
                     "DELAY", cpkP, cpUnsigned, &_delay,
                     "PERIOD", cpkP, cpUnsigned, &_period,
                     "PERIOD_HELLO", cpkP, cpUnsigned, &_periodHello,
                     "TIME_OUT", cpkP, cpUnsigned, &_time_out,
                     cpEnd) < 0) {
        return -1;
    }
    return 0;
}

void ReceiverTCP::run_timer(Timer *timer) {
    if(timer == &_timerTO){  // retransmit
        if(_my_state == CLOSED && !_finished_transmission){
		click_chatter("[ReceiverTCP]: Retransmit SYNACK");
            	WritablePacket *packet = Packet::make(0, 0, sizeof(struct TCP_Header), 0);
		memcpy((void *)(packet->data()), (const void *)(&_duplicate_packet), sizeof(struct TCP_Packet));
		output(0).push(packet);
            _timerTO.schedule_after_sec(_time_out);
        }
        else if(_my_state == FIN_WAIT){
                click_chatter("[ReceiverTCP]: Retransmit FIN");
                WritablePacket *packet = Packet::make(0, 0, sizeof(struct TCP_Header), 0);
                memcpy((void *)(packet->data()), (const void *)(&_duplicate_packet), sizeof(struct TCP_Packet));
            output(0).push(packet);
            _timerTO.schedule_after_sec(_time_out);
        }
    }
    else if(timer == &_timerHello){
        click_chatter("[ReceiverTCP]: Sending new Hello packet");
        output(0).push(CreateOtherPacket(HELLO, NULL));
//        _timerHello.schedule_after_sec(_periodHello);
    }
    else {
        assert(false);
    }
}

// Received Packets 能传到这里的包，都是精选集了！在buffer 里面有顺序地拿到的耶！
void ReceiverTCP::push(int port, Packet *income_packet) {
    assert(income_packet);
    struct TCP_Packet* packet = (struct TCP_Packet*)income_packet->data();
    struct TCP_Header* header = (struct TCP_Header*)(&(packet->header));
    WritablePacket* output_packet = NULL;
    
    if(header->type == DATA){
        click_chatter("[ReceiverTCP]: Received DATA: packet %u from %u", header->sequence, header->source);
        click_chatter("[ReceiverTCP-buffer]: Send ACK(%u) for DATA packet %u", _seq, header->sequence);
//        output(0).push(CreateOtherPacket(ACK, header));
        // Recover data【tcb】
    }
    else if(header->type == SYN){
        click_chatter("[ReceiverTCP]: Received SYN request: packet %u from %u", header->sequence, header->source);
        WritablePacket* packet_synack = CreateOtherPacket(SYNACK, header);
        memcpy((void *)(&_duplicate_packet), (const void *)(packet_synack->data()), sizeof(struct TCP_Packet)); // store a copy
        click_chatter("[ReceiverTCP-buffer]: Send SYNACK for SYN");
//        output(0).push(packet_synack);
        _timerTO.schedule_after_sec(_time_out);
    }
    else if(header->type == ACK){  // ACK for SYNACK
        click_chatter("[ReceiverTCP]: Received ACK for SYNACK(%u): packet %u from %u", header->ack, header->sequence, header->source);
        click_chatter("[ReceiverTCP]: ==============CONNECTION ESTABLISHED================");
        _my_state = CONNECTED;
        _timerTO.unschedule();
    }
    else if(header->type == FINACK){
        click_chatter("[ReceiverTCP]: Received FINACK for FIN(%u): packet %u from %u", header->ack, header->sequence, header->source);
        click_chatter("[SenderTCP]: =================CONNECTION TORN DOWN================");
        _my_state = CLOSED;
        _timerTO.unschedule();
    }
    else if(header->type == INFO){
//        click_chatter("[ReceiverTCP]: Received INFO from buffer.");
        _empty_receiver_buffer_size = header->empty_buffer_size;
//        click_chatter("[ReceiverTCP]: Room for %u packets in ReceiverBuffer", _empty_receiver_buffer_size);
    }
    else if(header->type == HELLO){
        click_chatter("[ReceiverTCP]: Received HELLO: packet %u from %u", header->sequence, header->source);
        // 好像没啥可干的……不会收到hello吧，应该在router就给drop掉了
    }
    else if(header->type == FIN){
        click_chatter("[ReceiverTCP]: Received FIN: packet %u from %u", header->sequence, header->source);
        click_chatter("[ReceiverTCP-buffer]: Send FINACK for FIN");
//        output(0).push(CreateOtherPacket(FINACK, header));
        _other_state = CLOSED;
        // send FIN for itself
        click_chatter("[ReceiverTCP]: Send FIN");
        WritablePacket* packet_fin = CreateOtherPacket(FIN, header);
        output(0).push(packet_fin);
        memcpy((void *)(&_duplicate_packet), (const void *)(packet_fin), sizeof(struct TCP_Packet)); // store a copy
        _timerTO.schedule_after_sec(_time_out);
    }
    
    // delete original packet
    income_packet->kill();
}

WritablePacket* ReceiverTCP::CreateOtherPacket(packet_types type_of_packet, TCP_Header* header){
    WritablePacket *packet = Packet::make(0, 0, sizeof(struct TCP_Header), 0);
    struct TCP_Packet* packet_ptr = (struct TCP_Packet*)packet->data();
    struct TCP_Header* header_ptr = (struct TCP_Header*)(&(packet_ptr->header));
    
    memset(packet_ptr, 0, packet->length());
    
    // Write TCP_Header
    header_ptr->type = type_of_packet;
    header_ptr->source = _my_address;
    header_ptr->destination = _other_address;
    header_ptr->sequence = _seq;
    _seq++;
    
    // Flow Control
    if(type_of_packet == ACK || type_of_packet == SYNACK || type_of_packet == FINACK){
        header_ptr->ack = header->sequence;
        header_ptr->empty_buffer_size = _empty_receiver_buffer_size;
    }
    return packet;
}

// Return whether reaches the end of file. 【tbc】
bool ReceiverTCP::ReadDataFromFile(char* ptr){
    return true;
}

CLICK_ENDDECLS
EXPORT_ELEMENT(ReceiverTCP)

