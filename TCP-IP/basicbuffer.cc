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
#include "BasicBuffer.hh"
#include "packets.hh"
#include "packet.hh"

CLICK_DECLS

BasicBuffer::BasicBuffer() {
    click_chatter("Creating a BasicBuffer object.");
}

BasicBuffer::~BasicBuffer(){
    click_chatter("Killing a BasicBuffer object.");
}

int BasicBuffer::initialize(ErrorHandler *errh){
    return 0;
}

void BasicBuffer::push(int port, Packet *income_packet) {
    assert(income_packet);
    struct TCP_Packet *packet = (struct TCP_Packet *)income_packet->data();
    struct TCP_Header header = (struct TCP_Header)packet->header;
    
    /* from TCP, use sender buffer */
    if(port == 0){
        // Store in buffer
        if(header.type == DATA || header.type == SYN || header.type == FIN){ // no need to store HELLO or ACK
            int TCP_Packet_size = sizeof(struct TCP_Packet);
            memcpy((void *)(_sender_buffer + _sender_end_pos * TCP_Packet_size), (const void *)income_packet, TCP_Packet_size);
            
            // update pointer
            _sender_end_pos = (_sender_end_pos + 1) % SENDER_BUFFER_SIZE;
            
            // inform TCP of the change in sender buffer
            output(0).push(CreateInfoPacket());
        }
        else if(header.type == RETRANS){
            Retrasmit();
        }
        
        // Route
        output(1).push(income_packet);
        output(0).push(CreateInfoPacket());  // back to TCP
    }
    
    /* from IP, use receiver buffer*/
    else if(port == 1){
        // 需要检查这个ack是不是对应着有效的包，检查sender buffer是不是空的相应地更新senderbuffer里面的状态
        if(header.type == ACK && !SenderBufferEmpty()){
            if(header.ack == GetFirstSeqInSenderBuffer()){
                // send ACK to TCP
                output(0).push(income_packet);
                
                // update pointer
                _sender_start_pos = (_sender_start_pos + 1) % SENDER_BUFFER_SIZE;
                
                // inform TCP of the change in senderbuffer
                output(0).push(CreateInfoPacket());
            }
            else{
                income_packet->kill();
            }
        }
        // 需要检查这个来的DATA是不是按顺序来的，检查receiver buffer里面有没有位置可以放，相应地更新receiverbuffer里面的状态
        else if((header.type == DATA || header.type == SYN || header.type == FIN) && !ReceiverBufferFull()){
            if(header.seq == GetLastSeqInReceiverBuffer() + 1){
                // store in receiver buffer
                int TCP_Packet_size = sizeof(struct TCP_Packet);
                memcpy((void *)(_receiver_buffer + _receiver_end_pos * TCP_Packet_size), (const void *)income_packet, TCP_Packet_size);
                
                // update pointer
                _receiver_end_pos = (_receiver_end_pos + 1) % RECEIVER_BUFFER_SIZE;
            }
            else{
                income_packet->kill();
            }
        }
        else{
            income_packet->kill();
        }
    }
}

void BasicTCP::run_timer(Timer *timer) {
    if (timer == &_timerSend){  // time to send packets to TCP
        if(!ReceiverBufferEmpty()){
            output(0).push(ReadOutDataPacket());
            _receiver_start_pos = (_receiver_start_pos + 1) % RECEIVER_BUFFER_SIZE;
        }
        _timerSend.schedule_after_sec(_send_interval);
    }
}


WritablePacket* BasicBuffer::CreateInfoPacket(){
    WritablePacket *packet = Packet::make(0, 0, sizeof(struct TCP_Packet), 0);
    struct TCP_Packet* packet_ptr = (struct TCP_Packet*)packet->data();
    struct TCP_Header* header_ptr = (struct TCP_Header*)(&(packet_ptr->header));
    
    memset(packet_ptr, 0, packet->length());
    
    // Write TCP_Header
    header_ptr->type = INFO;
    header_ptr->empty_buffer_size = SenderBufferRemainSize(_sender_start_pos, _sender_end_pos);
    
    return packet;
}

WritablePacket* BasicBuffer::ReadOutDataPacket(){
    // Readout data
    int TCP_Packet_size = sizeof(struct TCP_Packet);
    WritablePacket* packet = Packet::make(0, 0, sizeof(struct TCP_Packet), 0);
    memcpy((void *)packet, (const void *)(_receiver_buffer + _receiver_start_pos * TCP_Packet_size), TCP_Packet_size);
    
    // Update pointer
    _receiver_start_pos = (_receiver_start_pos + 1) % RECEIVER_BUFFER_SIZE;
    return packet;
}

uint8_t BasicBuffer::SenderBufferRemainSize(uint8_t s, uint8_t e){
    if(e >= s){
        return SENDER_BUFFER_SIZE - 1 - (e - s);
    }
    return s - e - 1;
}

uint8_t BasicBuffer::ReceiverBufferRemainSize(uint8_t s, uint8_t e){
    if(e >= s){
        return RECEIVER_BUFFER_SIZE - 1 - (e - s);
    }
    return s - e - 1;
}

// Return receive_buffer full or not
bool BasicBuffer::ReceiverBufferFull(){
    return _receiver_start_pos == (_receiver_end_pos + 1) % RECEIVER_BUFFER_SIZE;
}

// Return receive_buffer full or not
bool BasicBuffer::SenderBufferFull(){
    return _sender_start_pos == (_sender_end_pos + 1) % SENDER_BUFFER_SIZE;
}

bool BasicBuffer::ReceiverBufferEmpty(){
    return _receiver_start_pos == _receiver_end_pos;
}

bool BasicBuffer::SenderBufferEmpty(){
    return _sender_start_pos == _sender_end_pos;
}

uint32_t BasicBuffer::GetFirstSeqInSenderBuffer(){
    int TCP_Packet_size = sizeof(struct TCP_Packet);
    WritablePacket* packet = Packet::make(0, 0, sizeof(struct TCP_Packet), 0);
    struct TCP_Packet* packet_ptr = (struct TCP_Packet*)packet->data();
    memcpy((void *)(&packet_ptr), (const void *)(_sender_buffer + _sender_start_pos * TCP_Packet_size), TCP_Packet_size);
    struct TCP_Header* header_ptr = (struct TCP_Header*)(&(packet_ptr->header));
    return header_ptr->seq;
}

uint32_t BasicBuffer::GetLastSeqInReceiverBuffer(){
    int TCP_Packet_size = sizeof(struct TCP_Packet);
    WritablePacket* packet = Packet::make(0, 0, sizeof(struct TCP_Packet), 0);
    struct TCP_Packet* packet_ptr = (struct TCP_Packet*)packet->data();
    memcpy((void *)(&packet_ptr), (const void *)(_receiver_buffer + (_receiver_end_pos + RECEIVER_BUFFER_SIZE - 1) * TCP_Packet_size), TCP_Packet_size);
    struct TCP_Header* header_ptr = (struct TCP_Header*)(&(packet_ptr->header));
    return header_ptr->seq;
}

// retransmit all packets in buffer 【tbc】
void BasicBuffer::Retrasmit(){
    
}

CLICK_ENDDECLS
EXPORT_ELEMENT(BasicBuffer)

