/*
 1）如何建立三步握手？
     要发之前，看一下是不是建立好了连接！存一下tcb
     要动态分配端口吗？？？
 2）怎么写packet啊
 3）包的类型在哪里加的？？？难道是之后通过不同的发出去的口，知道到底是什么类？？？
 4）以后再实现用不同的端口，建立多个tcp连接？？？
     可是这个是tcp里面啊，怎么着也不可能在这里面有多个tcp连接啊
 5）在sender那边也有一个buffer。
     有两个指针在移动，如果又一个ack没收到那么就会一直重发！
     产生的output会先推到buffer里面。
 6）需要读出window size啥的。
 7）感觉transmission这个值可以放在header里面
     不不不✋
     可是transmission这个值是和每个packet对应的啊。要不在推进buffer里的时候，也推进去transmission好了，放在struct里面。
 8）一个天大的bug：delay可能是0！所以不能直接设置时间为delay的闹钟，不然会一直在发。
 9）没有出发timer却还是像个智障一样一直发。科科
 10）给ip那里包裹起来！
 11）给receiver和sender都加上buffer！
 12）tcp这里还没开始读文件呢……gg
 13）应该达到的效果是，一边发文件，另外一边能把文件给恢复出来
 14）检查ack得对不对
 15）时序怎么控制啊？？？知道window size之后怎么调节呢？？？
     每次收到ack之后都更新_window_size
 16）维护文件指针！记得关掉！
 17）每个packet0口推出，进入buffer
     需要检查buffer现在没有满，不然不能接受更多啊！buffer有一些反馈信息
     sender这边的buffer存的是之前发出去过的packet
 18）文件传输结束之后，就准备断开tcp连接啊
 19）createDataPacket()直接把packet推走，不用返回值了
 20）检查ack是不是正确的ack应该在buffer里面实现？
 21）connection status
 */

#include <click/config.h>
#include <click/confparse.hh>
#include <click/error.hh>
#include <click/timer.hh>
#include <click/packet.hh>
#include "basictcp.hh" 
#include "packets.hh"
#include "packet.hh"
#include "basicbuffer.hh"

CLICK_DECLS 

BasicTCP::BasicTCP() : _timerTO(this), _timerHello(this) {
    _seq = 0;
    _period = 3;
    _periodHello = 2;
    _delay = 0;
    _time_out = 1;
    _my_address = 0;
    _other_address = 0;
    transmissions = 0;
    _connection_setup = 0;
}

BasicTCP::~BasicTCP(){
    
}

int BasicTCP::initialize(ErrorHandler *errh){
    _timerTO.initialize(this);
    _timerHello.initialize(this);
    if(_delay>0){
        _timerTO.schedule_after_sec(_delay);
    }
    _timerHello.schedule_after_sec(_periodHello);
    return 0;
}

int BasicTCP::configure(Vector<String> &conf, ErrorHandler *errh) {
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

// Time out
void BasicTCP::run_timer(Timer *timer) {
    if(timer == &_timerTO){ // Time out. Retrasmit all the files in sender buffer
        //click_chatter("Time-Out!");
        if(_connection_setup == 0){
            click_chatter("Sending SYN %u. Trying to set up connection with %u", _seq, _other_address);
            WritablePacket* output_packet = CreateOtherPacket(SYN, NULL);
            output(0).push(output_packet);
            transmissions++;
            _timerTO.schedule_after_sec(_time_out);
        }
        else{
            if(transmissions == 0){
                click_chatter("Send packet %u", _seq);
            }
            else{
                click_chatter("Retransmitting packet %u for %d time", _seq, transmissions);
            }
            
            WritablePacket* output_packet = CreateDataPacket();
            output(0).push(output_packet);
            transmissions++;
            _timerTO.schedule_after_sec(_time_out);
        }
    }
    else if(timer == &_timerHello){
        click_chatter("Sending new Hello packet");
        WritablePacket* output_packet = CreateOtherPacket(HELLO, NULL);
        output(0).push(output_packet);
        _timerHello.schedule_after_sec(_periodHello);
    }
    else {
        assert(false);
    }
}

// Received Packets
void BasicTCP::push(int port, Packet *income_packet) {
    assert(income_packet);
    struct TCP_Packet* packet = (struct TCP_Packet*)income_packet->data();
    struct TCP_Header* header = (struct TCP_Header*)(&(packet->header));
    WritablePacket* output_packet = NULL;
    
    if(header->source == _my_address){
        income_packet->kill();
        return;
    }
    
    if(header->type == DATA){
        click_chatter("Received DATA: packet %u from %u", header->sequence, header->source);
        output_packet = CreateOtherPacket(ACK, header);
        // Recover data【tcb】
    }
    else if(header->type == SYN){
        click_chatter("Received SYN request: packet %u from %u", header->sequence, header->source);
        output_packet = CreateOtherPacket(ACK, header);
    }
    else if(header->type == ACK){
        _seq++;
        transmissions = 0;
        _timerTO.unschedule();
        _empty_receiver_buffer_size = header->empty_buffer_size;
        
        if(_connection_setup == 1){  // ACK for DATA
            click_chatter("Received ACK for DATA: packet %u from %u", header->sequence, header->source);
            output_packet = CreateDataPacket();
            _timerTO.schedule_after_sec(_period); // 这里当然是不对了！【注意】
        }
        else{  // ACK for SYN
            click_chatter("Received ACK for SYN: packet %u from %u. Connection Established!", header->sequence, header->source);
            output_packet = CreateOtherPacket(ACK, header);
            // 现在自己这边连接建立好了，更改状态，开始发送！
            // 这里是不是需要设置一个时钟，delay时间之后开始发data。【】
            _connection_setup = 1;
            if(_delay > 0){
                _timerTO.schedule_after_sec(_delay);
            }
        }
    }
    else if(header->type == INFO){
        click_chatter("Received INFO from buffer.");
        _empty_sender_buffer_size = header->empty_buffer_size;
    }
    else if(header->type == HELLO){
        click_chatter("Received HELLO: packet %u from %u", header->sequence, header->source);
        // 好像没啥可干的……不会收到hello吧，应该在router就给drop掉了
    }
    else if(header->type == FIN){
        click_chatter("Received FIN: packet %u from %u", header->sequence, header->source);
        // 开始四步握手
    }
    
    // delete original packet
    income_packet->kill();
    
    if(output_packet != NULL){ //&& ((struct TCP_Packet*)output_packet->data())->header.type == ACK){
        output(0).push(output_packet);
    }
}

WritablePacket* BasicTCP::CreateOtherPacket(packet_types type_of_packet, TCP_Header* header){
    WritablePacket *packet = Packet::make(0, 0, sizeof(struct TCP_Header), 0);
    struct TCP_Packet* packet_ptr = (struct TCP_Packet*)packet->data();
    struct TCP_Header* header_ptr = (struct TCP_Header*)(&(packet_ptr->header));
    
    memset(packet_ptr, 0, packet->length());
    
    // Write TCP_Header
    header_ptr->type = type_of_packet;
    header_ptr->source = _my_address;
    header_ptr->destination = _other_address;
    header_ptr->sequence = _seq;

    // Flow Control
    if(type_of_packet == ACK){
        header_ptr->ack = header->sequence;
        header_ptr->empty_buffer_size = _empty_receiver_buffer_size;
    }
    return packet;
}

// 【tbc】
void BasicTCP::CreateDataPacket(){
    _window_size = min(_empty_sender_buffer_size, _empty_receiver_buffer_size);
    
    for(int i = 0; i < _window_size;++i){
        WritablePacket *packet = CreateOtherPacket(DATA, NULL);
        struct TCP_Packet* packet_ptr = (struct TCP_Packet*)packet->data();
        char* data_ptr = &(packet_ptr->data);
        // Write TCP Data
        
        output(0).push(packet);
    }
    return packet;
}

// Return whether reaches the end of file. 【tbc】
bool BasicTCP::ReadDataFromFile(char* ptr){
    return true;
}

bool BasicTCP::NeedRetransmission(){
    return _empty_sender_buffer_size != (SENDER_BUFFER_SIZE - 1);
}

CLICK_ENDDECLS 
EXPORT_ELEMENT(BasicTCP)
