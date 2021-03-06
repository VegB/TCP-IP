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
 8）又是一个惊天大bug……memcpy的时候必须得是->data()这个指针。不然会出错。
 -------------
 9）是不是也应该改成在senderbuffer里面重发？
     感觉在tcp里面处理也可以。如果timeout之后发现buffer中有东西，那就全部重发？
 10）_last_acked这个还没设置！
 -------------
 11）slow start咋写啊？
 */

#include <click/config.h>
#include <click/confparse.hh>
#include <click/error.hh>
#include <click/timer.hh>
#include <click/packet.hh>
#include "senderbuffer.hh"
#include "packets.hh"
#include "packet.hh"

CLICK_DECLS

SenderBuffer::SenderBuffer() {
    click_chatter("[SenderBuffer]: Creating a SenderBuffer object.");
    _sender_start_pos = 0;
    _sender_end_pos = 0;
    _last_acked = 0;
    _last_acked_cnt = 0;
}

SenderBuffer::~SenderBuffer(){
    click_chatter("[SenderBuffer]: Killing a SenderBuffer object.");
}

int SenderBuffer::initialize(ErrorHandler *errh){
    return 0;
}

void SenderBuffer::push(int port, Packet *income_packet) {
    assert(income_packet);
    struct TCP_Packet *packet = (struct TCP_Packet *)income_packet->data();
    struct TCP_Header header = (struct TCP_Header)packet->header;
    
    if(port == 0){  /* from TCP, use sender buffer */
        /* Store in buffer */
        if(header.type == DATA || header.type == SYN || header.type == FIN){ // no need to store HELLO or ACK
            int TCP_Packet_size = sizeof(struct TCP_Packet);
            memcpy((void *)(_sender_buffer + _sender_end_pos * TCP_Packet_size), (const void *)packet, TCP_Packet_size);
            // click_chatter("[SenderBuffer]: Store packet %u in buffer at position %u.", header.sequence, _sender_end_pos);
            /* update pointer */
            _sender_end_pos = (_sender_end_pos + 1) % SENDER_BUFFER_SIZE;
            /* inform TCP of the change in sender buffer */
            output(0).push(CreateInfoPacket());
        }
        else if(header.type == RETRANS){
            RetransmitAll();
            income_packet->kill();
        }
        
        /* Route */
        if(header.type != RETRANS){
            click_chatter("[SenderBuffer]: Pass packet %u on to IP", header.sequence);
            output(1).push(income_packet);  // to IP
        }
        output(0).push(CreateInfoPacket());  // back to TCP
    }
    if(port == 1){  /* from IP */
        // 需要检查这个ack是不是对应着有效的包，检查RECEIVER buffer是不是空的,相应地更新ReceiverBuffer里面的状态
        if((header.type == ACK || header.type == SYNACK || header.type == FINACK) && !SenderBufferEmpty()){
            // click_chatter("[SenderBuffer]: Received SYN/SYNACK/FINACK for packet %u", header.ack);
            
            if(header.ack == _last_acked){
                _last_acked_cnt += 1;
                click_chatter("[SenderBuffer]: last_acked: %u, receiving for the %u th time.", _last_acked, _last_acked_cnt);
                if(_last_acked_cnt >= FAST_RETRANSMIT_BOUND){  // Fast Retransmit
                    Retransmit(_last_acked + 1);
                }
            }
            else if(header.ack > _last_acked){
                _last_acked_cnt = 0;
                _last_acked = header.ack;
                //click_chatter("[SenderBuffer]: update last_acked to %u", _last_acked);
                while(!SenderBufferEmpty() && GetSeqInSenderBuffer(_sender_start_pos) <= header.ack){
                    _sender_start_pos = (_sender_start_pos + 1) % SENDER_BUFFER_SIZE;
                }
                output(0).push(CreateInfoPacket()); // inform TCP of the change in ReceiverBuffer
            }
            
            /* send ACK to TCP */
            output(0).push(income_packet);
        }
        else if(header.type == FIN){
            output(0).push(income_packet);
        }
        else{
            income_packet->kill();
        }
    }
}

WritablePacket* SenderBuffer::CreateInfoPacket(){
    WritablePacket *packet = Packet::make(0, 0, sizeof(struct TCP_Packet), 0);
    struct TCP_Packet* packet_ptr = (struct TCP_Packet*)packet->data();
    struct TCP_Header* header_ptr = (struct TCP_Header*)(&(packet_ptr->header));
    
    memset(packet_ptr, 0, packet->length());
    
    /* Write TCP_Header */
    header_ptr->type = INFO;
    header_ptr->empty_buffer_size = SenderBufferRemainSize(_sender_start_pos, _sender_end_pos);
    
    // click_chatter("[SenderBuffer]: CreateInfoPacket(): start_pos: %u, end_pos: %u. Remain size: %u", _sender_start_pos, _sender_end_pos, header_ptr->empty_buffer_size);
    
    return packet;
}

uint32_t SenderBuffer::SenderBufferRemainSize(uint32_t s, uint32_t e){
    if(e >= s){
        return SENDER_BUFFER_SIZE - 1 - (e - s);
    }
    return s - e - 1;
}

/* Return receive_buffer full or not */
bool SenderBuffer::SenderBufferFull(){
    return _sender_start_pos == (_sender_end_pos + 1) % SENDER_BUFFER_SIZE;
}

bool SenderBuffer::SenderBufferEmpty(){
    return _sender_start_pos == _sender_end_pos;
}

uint32_t SenderBuffer::GetSeqInSenderBuffer(int pos){
    int TCP_Packet_size = sizeof(struct TCP_Packet);
    WritablePacket* packet = Packet::make(0, 0, sizeof(struct TCP_Packet), 0);
    struct TCP_Packet* packet_ptr = (struct TCP_Packet*)packet->data();
    memcpy((void *)(packet_ptr), (const void *)(_sender_buffer + pos * TCP_Packet_size), TCP_Packet_size);
    uint32_t seq = ((struct TCP_Header*)(&(packet_ptr->header)))->sequence;
    packet->kill();
    return seq;
}

/* Just getting each packet in buffer. */
WritablePacket* SenderBuffer::ReadOutDataPacket(int pos){
    /* Readout data */
    int TCP_Packet_size = sizeof(struct TCP_Packet);
    WritablePacket* packet = Packet::make(0, 0, sizeof(struct TCP_Packet), 0);
    memcpy((void *)(packet->data()), (const void *)(_sender_buffer + pos * TCP_Packet_size), TCP_Packet_size);

	struct TCP_Packet* packet_ptr = (struct TCP_Packet*)packet->data();
    struct TCP_Header* header_ptr = (struct TCP_Header*)(&(packet_ptr->header));
    click_chatter("[SenderBuffer]: Read out packet %u at position %d and retransmit.", header_ptr->sequence, pos);
    return packet;
}

/* retransmit packet with sequence number 'seq' in buffer */
void SenderBuffer::Retransmit(uint32_t seq){
    click_chatter("[SenderBuffer]: FAST RETRANSMIT");
    if(_sender_start_pos < _sender_start_pos){
        for(int i = _sender_start_pos; i < _sender_end_pos; ++i){
            if(GetSeqInSenderBuffer(i) == seq){
                output(1).push(ReadOutDataPacket(i));
                break;
            }
        }
    }
    else{
        for(int i = _sender_end_pos; i < SENDER_BUFFER_SIZE; ++i){
            if(GetSeqInSenderBuffer(i) == seq){
                output(1).push(ReadOutDataPacket(i));
                break;
            }
        }
        for(int i = 0; i < _sender_end_pos; ++i){
            if(GetSeqInSenderBuffer(i) == seq){
                output(1).push(ReadOutDataPacket(i));
                break;
            }
        }
    }
}

/* retransmit all packets remained in buffer */
void SenderBuffer::RetransmitAll(){
    click_chatter("[SenderBuffer]: Retransmit ALL");
    if(_sender_start_pos < _sender_end_pos){
        for(int i = _sender_start_pos; i < _sender_end_pos; ++i){
            output(1).push(ReadOutDataPacket(i));
        }
    }
    else{
        for(int i = _sender_start_pos; i < SENDER_BUFFER_SIZE; ++i){
            output(1).push(ReadOutDataPacket(i));
        }
        for(int i = 0; i < _sender_end_pos; ++i){
            output(1).push(ReadOutDataPacket(i));
        }
    }
}

CLICK_ENDDECLS
EXPORT_ELEMENT(SenderBuffer)

