#ifndef CLICK_BASICROUTER_HH 
#define CLICK_BASICROUTER_HH 
#include <click/element.hh>
#include <click/timer.hh>
#include <click/hashtable.hh>

CLICK_DECLS

class BasicRouter : public Element {
    public:
        BasicRouter();
        ~BasicRouter();
        const char *class_name() const { return "BasicRouter";}
        const char *port_count() const { return "1-/1-";}
        const char *processing() const { return PUSH; }
		
		void push(int port, Packet *packet);
        int initialize(ErrorHandler*);
		
	private:
		HashTable<int, int> _ports_table;
		
}; 

CLICK_ENDDECLS
#endif 
