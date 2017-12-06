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
 22）how to exit？？？
 23）简单起见，sender主动开始，主动结束
 24）一个特别重要的问题：现在的设定里面，receiver那边没有存着发了什么东西的buffer了。
     如果丢包了呢？（SYNACK, FIN）???
     像原来一样傻等着吧。
 25）现在的seq是在每次createpacket的时候更新的。
     但是对于那些内部传送的包（比如RETRANS之类的），seq不更新，不然就乱了套了
 */

#include <click/config.h>
#include <click/confparse.hh>
#include <click/error.hh>
#include <click/timer.hh>
#include <click/packet.hh>
#include "sendertcp.hh"
#include "packets.hh"
#include "packet.hh"
#include "senderbuffer.hh"

CLICK_DECLS 

SenderTCP::SenderTCP() : _timerTO(this), _timerHello(this) {
    click_chatter("[SenderTCP]: Creating a SenderTCP object.");
    _seq = 1;
    _period = 3;
    _periodHello = 2;
    _delay = 0;
    _time_out = 10;
    _my_address = 0;
    _other_address = 0;
    transmissions = 0;

	_window_size = 1;
	_offset = 0;
    _my_state = CLOSED;
	_other_state = CLOSED;
	_empty_sender_buffer_size = SENDER_BUFFER_SIZE - 1;
	_empty_receiver_buffer_size = 100000; // define big buffer
    _finished_transmission = 0;
	_data_piece_cnt = 0;
}

SenderTCP::~SenderTCP(){
	click_chatter("[SenderTCP]: Killing a SenderTCP object.");    
}

int SenderTCP::initialize(ErrorHandler *errh){
    _timerTO.initialize(this);
    _timerHello.initialize(this);
    if(_delay>0){
        _timerTO.schedule_after_sec(_delay);
    }
    _timerHello.schedule_after_sec(_periodHello);
    return 0;
}

int SenderTCP::configure(Vector<String> &conf, ErrorHandler *errh) {
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
void SenderTCP::run_timer(Timer *timer) {
    if(timer == &_timerTO){
        /* Time out. Retrasmit all the files in sender buffer */
        if(NeedRetransmission()){
            click_chatter("[SenderTCP]: Need Retransmission.");
            output(0).push(CreateOtherPacket(RETRANS, NULL));
        }
        else{  /* Send new packets. */
            if(_my_state == CLOSED && !_finished_transmission){
                click_chatter("[SenderTCP]: Sending SYN %u. Trying to set up connection with %u", _seq, _other_address);
                output(0).push(CreateOtherPacket(SYN, NULL));
            }
            else if(_my_state == CONNECTED){
                click_chatter("[SenderTCP]: -------Sending DATA packets-------");
                CreateDataPacket();
            }
            else if(_my_state == FIN_WAIT){
                click_chatter("[SenderTCP]: Sending FIN %u. Trying to release connection with %u", _seq, _other_address);
                output(0).push(CreateOtherPacket(FIN, NULL));
            }
            else{
                click_chatter("[SenderTCP]: Data transmission probably finished!");
                // exit();
            }
        }
        // schedule timer for next time out
        if(!_finished_transmission){
		    // click_chatter("[SenderTCP]: Set up timer for TO.");
            _timerTO.schedule_after_sec(_time_out);
        }
    }
    else if(timer == &_timerHello){
        click_chatter("[SenderTCP]: Sending new Hello packet");
        output(0).push(CreateOtherPacket(HELLO, NULL));
        // _timerHello.schedule_after_sec(_periodHello);
    }
    else {
        assert(false);
    }
}

// Received Packets
void SenderTCP::push(int port, Packet *income_packet) {
    assert(income_packet);
    struct TCP_Packet* packet = (struct TCP_Packet*)income_packet->data();
    struct TCP_Header* header = (struct TCP_Header*)(&(packet->header));
    WritablePacket* output_packet = NULL;
    
    if(header->type == ACK){  // 这里接受到的ACK是经过buffer审核的（只有那些待收的ack才会发上来)_my_state == CONNECTED or FIN_WAIT
        click_chatter("[SenderTCP]: Received ACK for DATA(%u): packet %u from %u", header->ack, header->sequence, header->source);
        
        // record valid size in receiver buffer
        _empty_receiver_buffer_size = header->empty_buffer_size;
        click_chatter("[SenderTCP]: Room for %u packets in ReceiverBuffer", _empty_receiver_buffer_size);
    }
    else if(header->type == SYNACK){
        click_chatter("[SenderTCP]: Received SYNACK for SYN(%u): packet %u from %u", header->ack, header->sequence, header->source);
        click_chatter("[SenderTCP]: =================CONNECTION ESTABLISHED================");
        _my_state = CONNECTED;
        output(0).push(CreateOtherPacket(ACK, header));
    }
    else if(header->type == FINACK){
        click_chatter("[SenderTCP]: Received FINACK for SYN(%u): packet %u from %u", header->ack, header->sequence, header->source);
        click_chatter("[SenderTCP]: =================CONNECTION TORN DOWN================");
        _my_state = CLOSED;
        _finished_transmission = 1;
    }
    else if(header->type == INFO){
        // click_chatter("[SenderTCP]: Received INFO from SenderBuffer.");
        _empty_sender_buffer_size = header->empty_buffer_size;
        // click_chatter("[SenderTCP]: Room for %u packets in SenderBuffer", _empty_sender_buffer_size);
    }
    else if(header->type == HELLO){
        click_chatter("[SenderTCP]: Received HELLO: packet %u from %u", header->sequence, header->source);
        // 好像没啥可干的……不会收到hello吧，应该在router就给drop掉了
    }
    else if(header->type == FIN){
        click_chatter("[SenderTCP]: Received FIN: packet %u from %u", header->sequence, header->source);
        output(0).push(CreateOtherPacket(FINACK, header));
        _other_state = CLOSED;
    }
    
    // delete original packet
    income_packet->kill();
}

WritablePacket* SenderTCP::CreateOtherPacket(packet_types type_of_packet, TCP_Header* header){
    WritablePacket *packet = Packet::make(0, 0, sizeof(struct TCP_Header), 0);
    struct TCP_Packet* packet_ptr = (struct TCP_Packet*)packet->data();
    struct TCP_Header* header_ptr = (struct TCP_Header*)(&(packet_ptr->header));
    
    memset(packet_ptr, 0, packet->length());
    
    // Write TCP_Header
    header_ptr->type = type_of_packet;
    header_ptr->source = _my_address;
    header_ptr->destination = _other_address;
    if(type_of_packet == SYN || type_of_packet == FIN || type_of_packet == DATA || type_of_packet == SYNACK || type_of_packet == FINACK){
        header_ptr->sequence = _seq;
        _seq++;
    }
    
    // Flow Control
    if(type_of_packet == ACK || type_of_packet == SYNACK || type_of_packet == FINACK){
        header_ptr->ack = header->sequence;
        //header_ptr->empty_buffer_size = _empty_receiver_buffer_size;
    }
    return packet;
}

// send "window_size" piece of TCP packets
void SenderTCP::CreateDataPacket(){
    _window_size = _empty_sender_buffer_size < _empty_receiver_buffer_size? _empty_sender_buffer_size: _empty_receiver_buffer_size;
	click_chatter("[SenderTCP]: window_size = %u, SenderBuffer = %u, ReceiverBuffer = %u", _window_size, _empty_sender_buffer_size, _empty_receiver_buffer_size);	
    
    for(int i = 0; i < _window_size; ++i){
        WritablePacket *packet = CreateOtherPacket(DATA, NULL);
        struct TCP_Packet* packet_ptr = (struct TCP_Packet*)packet->data();
        struct TCP_Header* header_ptr = (struct TCP_Header*)(&(packet_ptr->header));
        header_ptr->more_packets = !(ReadDataFromFile());
        uint8_t _more = header_ptr->more_packets;
        click_chatter("[SenderTCP]: Have more packets? %u", _more);
        // pass it on to sender buffer
        output(0).push(packet);
        
        // might change connection state
        if(!_more){
            _my_state = FIN_WAIT; // send FIN later or it might get lost
            break;
        }
    }
}

// Return whether reaches the end of file. 【tbc】
bool SenderTCP::ReadDataFromFile(){
	_data_piece_cnt += 1;
    return _data_piece_cnt == 10;
}

bool SenderTCP::NeedRetransmission(){
    return _empty_sender_buffer_size != (SENDER_BUFFER_SIZE - 1);
}

CLICK_ENDDECLS 
EXPORT_ELEMENT(SenderTCP)
