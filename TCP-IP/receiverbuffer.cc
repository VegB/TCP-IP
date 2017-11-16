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
 ----------
 8）现在希望能够乱序接受！加入排序结构吧！
 9）click既然是c++架构，那么应该可以用map吧
 10）每次更新的时候，复制一遍目前的buffer中的编号到store中，排序，然后写回到buffer当中·
 11）等等这就很gg啊。到底是谁发的ack？按照这个设计，好像是buffer发的ack啊
 12）取消_timeSend这个闹钟
 13）在对buffer排序的过程中，需要去除掉重复的packets！
 14) 垃圾 弄了一晚是因为没有用income_packet->data()
 15) 一个很神棍的地方在于……没建立连接的时候就开始往buffer中写东西了
 */

#include <click/config.h>
#include <click/confparse.hh>
#include <click/error.hh>
#include <click/timer.hh>
#include <click/packet.hh>
#include <map>
#include <vector>
#include <algorithm>
#include "receiverbuffer.hh"
#include "packets.hh"
#include "packet.hh"
using namespace std;
CLICK_DECLS

ReceiverBuffer::ReceiverBuffer() {
    click_chatter("Creating a ReceiverBuffer object.");
    _receiver_start_pos = 0;
    _receiver_end_pos = 0;
    _last_acked = 0;
}

ReceiverBuffer::~ReceiverBuffer(){
    click_chatter("Killing a ReceiverBuffer object.");
}

int ReceiverBuffer::initialize(ErrorHandler *errh){
    return 0;
}

/* store a packet into receiver buffer */
void ReceiverBuffer::store_in_buffer(Packet *income_packet){
    struct TCP_Packet *packet = (struct TCP_Packet *)income_packet->data();
    struct TCP_Header header = (struct TCP_Header)packet->header;
    int TCP_Packet_size = sizeof(struct TCP_Packet);
    memcpy((void *)(_receiver_buffer + _receiver_end_pos * TCP_Packet_size), (const void *)packet, TCP_Packet_size);
    _receiver_end_pos = (_receiver_end_pos + 1) % RECEIVER_BUFFER_SIZE;
}

/* sort buffer, update _last_acked */
void ReceiverBuffer::sort_buffer(){
    map<int, int> store;  // {key:seq, val:pos}, stores the original position of 'seq'
    vector<int> seqes;
    uint32_t seq;
    /* sort */
    if(_receiver_end_pos > _receiver_start_pos){
        for(int i = _receiver_start_pos; i < _receiver_end_pos; ++i){
            seq = GetSeqInReceiverBuffer(i);
            if(store.find(seq) == store.end()){  // no duplicated 'seq'
                store[seq] = i;
                seqes.push_back(seq);
            }
        }
        sort(seqes.begin(), seqes.end());
    }
    else{
        for(int i = _receiver_start_pos; i < RECEIVER_BUFFER_SIZE; ++i){
            seq = GetSeqInReceiverBuffer(i);
            if(store.find(seq) == store.end()){  // no duplicated 'seq'
                store[seq] = i;
                seqes.push_back(seq);
            }
        }
        for(int i = 0; i < _receiver_end_pos; ++i){
            seq = GetSeqInReceiverBuffer(i);
            if(store.find(seq) == store.end()){  // no duplicated 'seq'
                store[seq] = i;
                seqes.push_back(seq);
            }
        }
        sort(seqes.begin(), seqes.end());
    }
    
    /* update ReceiverBuffer */
    _receiver_start_pos = 0;
    _receiver_end_pos = 0;
    int TCP_Packet_size =sizeof(struct TCP_Packet);
    memcpy((void*)_backup_buffer, (const void*)_receiver_buffer, RECEIVER_BUFFER_SIZE * TCP_Packet_size);
    for(int i = 0; i < seqes.size(); ++i){
        int ori_pos = store[seqes[i]];
        click_chatter("i: %u, ori_pos: %u", i, ori_pos);
        memcpy((void*)(_receiver_buffer + i * TCP_Packet_size), (const void*)(_backup_buffer + ori_pos * TCP_Packet_size), TCP_Packet_size);
        _receiver_end_pos += 1;
    }
	click_chatter("start_pos: %u, end_pos: %u", _receiver_start_pos, _receiver_end_pos);
}

/* collect packets in roll and pass them on to TCP, update pointer */
void ReceiverBuffer::send_packets_to_tcp(){
    for(int i = 0; i < _receiver_end_pos; ++i){
        uint32_t seq = GetSeqInReceiverBuffer(i);
        if(seq == _last_acked + 1){
            click_chatter("[ReceiverBuffer]: Send packet %u at pos %u to TCP.", seq, i);
            _last_acked += 1;
            _receiver_start_pos += 1;
            output(0).push(ReadOutDataPacket(i));
        }
        else{
            break;
        }
    }
}

/* received a new packet and update ReceiverBuffer accordingly */
void ReceiverBuffer::update_buffer(Packet *income_packet){
    store_in_buffer(income_packet);
    sort_buffer();
    send_packets_to_tcp();
    output(0).push(CreateInfoPacket()); // inform TCP of the change in ReceiverBuffer
}

void ReceiverBuffer::push(int port, Packet *income_packet) {
    assert(income_packet);
    struct TCP_Packet *packet = (struct TCP_Packet *)income_packet->data();
    struct TCP_Header header = (struct TCP_Header)packet->header;
	string packet_names[] = {"DATA", "ACK", "SYN", "SYNACK", "FIN", "FINACK", "HELLO", "INFO", "RETRANS"};    

    /* from TCP */
    if(port == 0){
        output(1).push(income_packet);  // pass on to IP
        click_chatter("[ReceiverBuffer]: Pass packet %u to IP", header.sequence);
    }    
    /* from IP, use receiver buffer */
    if(port == 1){
        if(header.type == ACK || header.type == FINACK){
            click_chatter("[ReceiverBuffer]: Received %s packet %u.", packet_names[header.type].c_str(), header.sequence);
            output(0).push(income_packet);  // send to TCP anyway(can only be ACK for SYNACK or FINACK)
        }
        else if((header.type == DATA || header.type == SYN || header.type == FIN) && !ReceiverBufferFull()){
            click_chatter("[ReceiverBuffer]: Received %s packet %u.", packet_names[header.type].c_str(), header.sequence);
	    // click_chatter("last acked: %u, received seq: %u", _last_acked, header.sequence);
            if(header.sequence > _last_acked){  // a packet that has not been given to tcp
                click_chatter("[ReceiverBuffer]: Packet %u not been given to TCP.", header.sequence);
                update_buffer(income_packet);
            }
            output(1).push(CreateAckPacket(&header));  // send ACK anyway
        }
        else{
            income_packet->kill();
            click_chatter("[ReceiverBuffer]: Received 'WRONG' packet %u. Buffer might be full.", header.ack);
        }
    }
}


WritablePacket* ReceiverBuffer::ReadOutDataPacket(int pos){
    // Readout data
    int TCP_Packet_size = sizeof(struct TCP_Packet);
    WritablePacket* packet = Packet::make(0, 0, sizeof(struct TCP_Packet), 0);
    memcpy((void *)(packet->data()), (const void *)(_receiver_buffer + pos * TCP_Packet_size), TCP_Packet_size);

    struct TCP_Packet* packet_ptr = (struct TCP_Packet*)packet->data();
    struct TCP_Header* header_ptr = (struct TCP_Header*)(&(packet_ptr->header));
    click_chatter("[ReceiverBuffer]: Read out packet %u at position %d", header_ptr->sequence, pos);

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

uint32_t ReceiverBuffer::GetSeqInReceiverBuffer(int pos){
    int TCP_Packet_size = sizeof(struct TCP_Packet);
    WritablePacket* packet = Packet::make(0, 0, sizeof(struct TCP_Packet), 0);
    struct TCP_Packet* packet_ptr = (struct TCP_Packet*)packet->data();
    memcpy((void *)(packet_ptr), (const void *)(_receiver_buffer + pos * TCP_Packet_size), TCP_Packet_size);
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

//    click_chatter("[ReceiverBuffer]: CreateInfoPacket(): start_pos: %u, end_pos: %u. Remain size: %u", _receiver_start_pos, _receiver_end_pos, header_ptr->empty_buffer_size);

    return packet;
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

CLICK_ENDDECLS
EXPORT_ELEMENT(ReceiverBuffer)

