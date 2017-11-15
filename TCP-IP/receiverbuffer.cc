/*
 1）是不是考虑发一些包回去给tcp，告知还有多少空位
 INFO PACKET?
 2）初始化！
 3）sender buffer这边一旦缺了一个，就会把所有的都重发？？？每次往里面填
 4）receiver buffer那里干的事情是，维持一个last_received_id，记录最后一个ack过的DATA包的id。
 如果来的包的seq比ack小，那么就直接扔掉
 5）sender那边在建立了syn之后，应该就只发data了。hello的包不给计数怎么样。
 6）现在的情况是，receiver那边如果有哪个包提前到了，就会被丢掉……
 7）是否重发是buffer和tcp双向沟通的结果：
 buffer会在info packet中告知tcp目前buffer中的状态
 tcp则会在retrans packet中通知buffer是否要重传目前buffer中的内容
 */

#include <click/config.h>
#include <click/confparse.hh>
#include <click/error.hh>
#include <click/timer.hh>
#include <click/packet.hh>
#include "receiverbuffer.hh"
#include "packets.hh"
#include "packet.hh"

CLICK_DECLS

ReceiverBuffer::ReceiverBuffer() : _timerSend(this) {
    click_chatter("Creating a ReceiverBuffer object.");
    _receiver_start_pos = 0;
    _receiver_end_pos = 0;
    _expected_seq = 0;
    _send_interval = 1;
}

ReceiverBuffer::~ReceiverBuffer(){
    click_chatter("Killing a ReceiverBuffer object.");
}

int ReceiverBuffer::initialize(ErrorHandler *errh){
    _timerSend.initialize(this);
	_timerSend.schedule_after_sec(_send_interval);
    return 0;
}

void ReceiverBuffer::push(int port, Packet *income_packet) {
    assert(income_packet);
    struct TCP_Packet *packet = (struct TCP_Packet *)income_packet->data();
    struct TCP_Header header = (struct TCP_Header)packet->header;

    if(port == 0){  /* from TCP */
        output(1).push(income_packet);  // pass on to IP
        click_chatter("[ReceiverBuffer]: Pass packet %u to IP", header.sequence);
    }    
    /* from IP, use receiver buffer*/
    if(port == 1){
        // 需要检查这个ack是不是对应着有效的包，检查RECEIVER buffer是不是空的,相应地更新ReceiverBuffer里面的状态
        if(header.type == ACK || header.type == FINACK){
            output(0).push(income_packet);  // send to TCP anyway(can only be ACK for SYNACK or FINACK)
        }        
        // 需要检查这个来的DATA是不是按顺序来的，检查receiver buffer里面有没有位置可以放，相应地更新receiverbuffer里面的状态
        else if((header.type == DATA || header.type == SYN || header.type == FIN) && !ReceiverBufferFull()){
            if(header.sequence == _expected_seq){
                // store in receiver buffer
                int TCP_Packet_size = sizeof(struct TCP_Packet);
                memcpy((void *)(_receiver_buffer + _receiver_end_pos * TCP_Packet_size), (const void *)packet, TCP_Packet_size);
                _expected_seq = header.sequence + 1;
		click_chatter("[ReceiverBuffer]: Received packet %u, store at position %u", header.sequence, _receiver_end_pos);
                
                // update pointer
                _receiver_end_pos = (_receiver_end_pos + 1) % RECEIVER_BUFFER_SIZE;
		// inform TCP of the change in ReceiverBuffer
                output(0).push(CreateInfoPacket());
            }
            else{
                income_packet->kill();
                click_chatter("[ReceiverBuffer]: Received 'UNWANTED' packet %u, expected %u", header.ack, _expected_seq);
            }
        }
        else{
            income_packet->kill();
            click_chatter("[ReceiverBuffer]: Received 'WRONG' packet %u. Buffer might be full.", header.ack);
        }
    }
}

void ReceiverBuffer::run_timer(Timer *timer) {
    if (timer == &_timerSend){  // time to send packets stored in ReceiverBuffer to TCP
        if(!ReceiverBufferEmpty()){
            output(0).push(ReadOutDataPacket());
        	output(0).push(CreateInfoPacket());
	}
        _timerSend.schedule_after_sec(_send_interval);
    }
}

WritablePacket* ReceiverBuffer::ReadOutDataPacket(){
    // Readout data
    int TCP_Packet_size = sizeof(struct TCP_Packet);
    WritablePacket* packet = Packet::make(0, 0, sizeof(struct TCP_Packet), 0);
    memcpy((void *)(packet->data()), (const void *)(_receiver_buffer + _receiver_start_pos * TCP_Packet_size), TCP_Packet_size);

        struct TCP_Packet* packet_ptr = (struct TCP_Packet*)packet->data();
        struct TCP_Header* header_ptr = (struct TCP_Header*)(&(packet_ptr->header));
    click_chatter("[ReceiverBuffer]: Read out packet %u at position %d", header_ptr->sequence, _receiver_start_pos);

    // Update pointer
    _receiver_start_pos = (_receiver_start_pos + 1) % RECEIVER_BUFFER_SIZE;
    return packet;
}

uint8_t ReceiverBuffer::ReceiverBufferRemainSize(uint8_t s, uint8_t e){
    if(e >= s){
        return RECEIVER_BUFFER_SIZE - 1 - (e - s);
    }
    return s - e - 1;
}

// Return receive_buffer full or not
bool ReceiverBuffer::ReceiverBufferFull(){
    return _receiver_start_pos == (_receiver_end_pos + 1) % RECEIVER_BUFFER_SIZE;
}

bool ReceiverBuffer::ReceiverBufferEmpty(){
    return _receiver_start_pos == _receiver_end_pos;
}

uint32_t ReceiverBuffer::GetFirstSeqInReceiverBuffer(){
    int TCP_Packet_size = sizeof(struct TCP_Packet);
    WritablePacket* packet = Packet::make(0, 0, sizeof(struct TCP_Packet), 0);
    struct TCP_Packet* packet_ptr = (struct TCP_Packet*)packet->data();
    memcpy((void *)(packet_ptr), (const void *)(_receiver_buffer + _receiver_start_pos * TCP_Packet_size), TCP_Packet_size);
    uint32_t seq = ((struct TCP_Header*)(&(packet_ptr->header)))->sequence;
    packet->kill();
    return seq;
}

WritablePacket* ReceiverBuffer::CreateInfoPacket(){
    WritablePacket *packet = Packet::make(0, 0, sizeof(struct TCP_Packet), 0);
    struct TCP_Packet* packet_ptr = (struct TCP_Packet*)packet->data();
    struct TCP_Header* header_ptr = (struct TCP_Header*)(&(packet_ptr->header));
    
    memset(packet_ptr, 0, packet->length());
    
    // Write TCP_Header
    header_ptr->type = INFO;
    header_ptr->empty_buffer_size = ReceiverBufferRemainSize(_receiver_start_pos, _receiver_end_pos);

    click_chatter("[ReceiverBuffer]: CreateInfoPacket(): start_pos: %u, end_pos: %u. Remain size: %u", _receiver_start_pos, _receiver_end_pos, header_ptr->empty_buffer_size);

    return packet;
}

CLICK_ENDDECLS
EXPORT_ELEMENT(ReceiverBuffer)


