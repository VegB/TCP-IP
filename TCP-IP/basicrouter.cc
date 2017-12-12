#include <click/config.h>
#include <click/confparse.hh>
#include <click/error.hh>
#include <click/timer.hh>
#include <click/packet.hh>
#include "basicrouter.hh" 
#include "packet.hh"
#define MAX 100000000
CLICK_DECLS 

BasicRouter::BasicRouter(): timerHello(this), timerRouting(this), timerQueuePop(this) {
	periodHello = 1;
	periodRouting = 5;
	periodQueuePop = 0.1;
	roundHello = 0;
	roundRouting = 0;
	myIP = 0;
	nodeNum = 0;
	outPort = 0;
	idHello = 0;
	portTable = NULL;
	forwardTable = NULL;
	prev = NULL;
	dist = NULL;
	topology = NULL;
	updated = NULL;
	ECNthre = 10;

}

BasicRouter::~BasicRouter(){
	delete[] portTable;
	delete[] forwardTable;
	delete[] prev;
	delete[] dist;
	for (int i = 0; i < nodeNum; i++)
		delete[] topology[i];
	delete[] topology;
	for (int i = 0; i < nodeNum; i++)
		delete[] updated[i];
	delete[] updated;
}

int BasicRouter::initialize(ErrorHandler *errh){
	portTable = new int[nodeNum];
	forwardTable = new int[nodeNum];
	prev = new int[nodeNum];
	dist = new int[nodeNum];
	topology = new int *[nodeNum];
	for (int i = 0; i < nodeNum; i++) {
		topology[i] = new int[nodeNum];
	}
	updated = new int *[nodeNum];
	for (int i = 0; i < nodeNum; i++) {
		updated[i] = new int[nodeNum];
	}
	for (int i = 0; i < nodeNum; i++) {
		portTable[i] = -1;
		forwardTable[i] = -1;
	}
	for (int i = 0; i < nodeNum; i++) {
		for (int j = 0; j < nodeNum; j++) {
			if (i == j) topology[i][j] = 0;
			else topology[i][j] = MAX;
			updated[i][j] = 0;
		}
	}
	timerHello.initialize(this);
	timerRouting.initialize(this);
	timerQueuePop.initialize(this);
	timerHello.schedule_after_sec(periodHello);
	timerRouting.schedule_after_sec(periodRouting);
	timerQueuePop.schedule_after_sec(periodQueuePop);
    return 0;
}

int BasicRouter::configure(Vector<String> &conf, ErrorHandler *errh) {
	if (cp_va_kparse(conf, this, errh,
		"MY_IP", cpkP, cpUnsigned, &myIP,
		"OUT_PORT_NUM", cpkP, cpUnsigned, &outPort,
		"TOTAL_NODE_NUM", cpkP, cpUnsigned, &nodeNum,
		"PERIOD_HELLO", cpkP, cpUnsigned, &periodHello,
		"PERIOD_ROUTING", cpkP, cpUnsigned, &periodRouting,
		cpEnd) < 0) {
		return -1;
	}
	return 0;
}
void BasicRouter::run_timer(Timer *timer) {
	if (timer == &timerQueuePop) {
		int size = packetQueue.size();
		if (size == 0) {
			timerQueuePop.schedule_after_sec(periodQueuePop);
			return;
		}
		else {
			click_chatter("[Router %u] Queue size = %d", myIP, size);
			Packet * packet = packetQueue.front();
			packetQueue.pop();
			struct IP_Header *header = (struct IP_Header *)packet->data();
			if (size > ECNthre) {
				header->ECN = true;
				header->ecn_limit = ECNthre;
                click_chatter("[Router %u]: Exceed ECN threshold! Start ECN!", myIP);
			}
			if (forwardTable[header->destination] == -1) {
				click_chatter("[Router %u] Fail to transfer Data from %u to destination %u because of disconnection with %u", myIP, header->source, header->destination, header->destination);
			}
			else {
				int next_port = portTable[forwardTable[header->destination]];
				output(next_port).push(packet);
			}
		}
		timerQueuePop.schedule_after_sec(periodQueuePop);
	}
	else if (timer == &timerHello) {
		click_chatter("[Router %u] Sends Hello to adjacent nodes for the %u-th time", myIP, ++roundHello);
		WritablePacket *packet = Packet::make(0, 0, sizeof(struct IP_Header), 0);
		memset(packet->data(), 0, packet->length());
		struct IP_Header *format = (struct IP_Header*) packet->data();
		format->type = HELLO;
		format->source = myIP;
		format->size = 1;
		//output(0).push(packet);
		for (int i = 0; i < outPort; i++) {
			Packet *tmpPacket = packet->clone();
			((struct IP_Header *)tmpPacket->data())->sequence = ++idHello;
			output(i).push(tmpPacket);
		}
		packet->kill();
		timerHello.schedule_after_sec(periodHello);
	}
	else if (timer == &timerRouting){
		click_chatter("[Router %u] ========== Recalculates routing table for the %u-th time ==========", myIP, ++roundRouting);
		bool *flag=new bool[nodeNum];
		for (int i = 0; i<nodeNum; ++i){
			dist[i] = topology[myIP][i];
			flag[i] = 0;
			if (dist[i] == MAX)
				prev[i] = -1;
			else
				prev[i] = myIP;
		}
		dist[myIP] = 0;
		flag[myIP] = 1;

		for (int i = 0; i<nodeNum - 1; ++i){
			int tmp = MAX;
			int u = myIP;
			// 找出当前未使用的点j的dist[j]最小值
			for (int j = 0; j < nodeNum; ++j) {
				if ((!flag[j]) && dist[j] < tmp){
					u = j;              // u保存当前邻接点中距离最小的点的号码
					tmp = dist[j];
				}
			}
			flag[u] = 1;    // 表示u点已存入S集合中

			// 更新dist
			for (int j = 0; j < nodeNum; ++j) {
				if ((!flag[j]) && topology[u][j] < MAX){
					int newdist = dist[u] + topology[u][j];
					if (newdist < dist[j]){
						dist[j] = newdist;
						prev[j] = u;
					}
				}
			}
		}
		
		int * next = new int[nodeNum];
		for (int u = 0; u < nodeNum; u++) {
			if (prev[u] == -1) {
				next[u] = -1;
			}
			else {
				int que[nodeNum + 5];
				int tot = 1;
				que[tot] = u;
				tot++;
				int tmp = prev[u];
				while (tmp != myIP){
					que[tot] = tmp;
					tot++;
					tmp = prev[tmp];
				}
				que[tot] = myIP;
				next[u] = que[tot - 1];
			}
		}
		
		bool change = false;
		for (int i = 0; i < nodeNum; ++i)
			if (next[i] != forwardTable[i]) change = true;
		if (change){
			click_chatter("[Router %u] Forwarding table is updated",myIP);
			for (int i = 0; i < nodeNum; ++i)
				click_chatter("(%d %d)\n", i, next[i]);
			for (int i = 0; i < nodeNum; ++i)
				forwardTable[i] = next[i];
		}
		else{
for (int i = 0; i < nodeNum; ++i)
                                click_chatter("(%d %d)\n", i, next[i]);
                        for (int i = 0; i < nodeNum; ++i)
                                forwardTable[i] = next[i];		
	click_chatter("[Router %u] But nothing changed",myIP);
		}
		for (int i = 0; i < nodeNum; i++) {
			for (int j = 0; j < nodeNum; j++) {
				if (i == j) topology[i][j] = 0;
				else topology[i][j] = MAX;
				updated[i][j] = 0;
			}
		}
		timerRouting.schedule_after_sec(periodRouting);
	}
	else {
		assert(false);
	}
}
void BasicRouter::push(int port, Packet *packet) {
	assert(packet);
	struct IP_Header *header = (struct IP_Header *)packet->data();
	if(header->type == DATA || header->type == ACK || header->type == SYN || header->type == SYNACK || header->type == FIN || header->type == FINACK) {
		click_chatter("[Router %u] Received Data from %u with destination %u",myIP, header->source, header->destination);
		packetQueue.push(packet);
		
	}
	else if (header->type == HELLO || header->type == EDGE) {
		if (header->type == HELLO) {
			click_chatter("[Router %u] Received Hello from %u on port %d with seq %u",myIP, header->source, port, header->sequence);
			portTable[header->source] = port;
			header->destination = myIP;
			header->type = EDGE;
		}
		if (header->sequence > updated[header->source][header->destination]) {
			click_chatter("[Router %u] Receive EDGE from %u to %u",myIP, header->source, header->destination);
			updated[header->source][header->destination] = header->sequence;
			topology[header->source][header->destination] = 1;
			topology[header->destination][header->source] = 1;
			for (int i = 0; i < outPort; i++) {
				output(i).push(packet->clone());
			}
			packet->kill();
		}
	}
	else {
		click_chatter("Wrong packet type");
		packet->kill();
	}
}

CLICK_ENDDECLS 
EXPORT_ELEMENT(BasicRouter)
