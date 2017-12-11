#ifndef CLICK_BASICROUTER_HH 
#define CLICK_BASICROUTER_HH 
#include <click/element.hh>
#include <click/timer.hh>
#include <click/hashtable.hh>
#include <click/packet.hh>
#include <queue>
using namespace std;

CLICK_DECLS

class BasicRouter : public Element {
    public:
        BasicRouter();
        ~BasicRouter();
        const char *class_name() const { return "BasicRouter";}
        const char *port_count() const { return "1-/1-";}
        const char *processing() const { return PUSH; }

		int configure(Vector<String> &conf, ErrorHandler *errh);
		void run_timer(Timer *timer);
		void push(int port, Packet *packet);
        int initialize(ErrorHandler*);
		
	private:
		int * portTable; //<IP address, port>
		int * forwardTable; //<destination IP, forward IP>
		int * prev;
		int * dist;
		int ** topology;
		int ** updated;
		Timer timerHello;
		Timer timerRouting;
		Timer timerQueuePop;
		int periodHello;
		int periodRouting;
		int periodQueuePop;
		int roundHello;
		int roundRouting;
		int myIP;
		int nodeNum;
		int outPort;
		int idHello;
		queue<Packet *> packetQueue;
		int ECNthre;
		
}; 

CLICK_ENDDECLS
#endif 
