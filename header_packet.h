#ifndef HEADER_PACKET_H
#define HEADER_PACKET_H


#include "rqueue.h"
#include "md5.h"


struct sHeaderPacket {
	void *stack;
	u_int16_t packet_alloc_size;
	u_int8_t _detect_headers;
	u_int16_t _header_ip_first_offset;
	u_int16_t _header_ip_offset;
	int _eth_protocol;
	uint16_t _md5[MD5_DIGEST_LENGTH / (sizeof(uint16_t) / sizeof(unsigned char))];
	pcap_pkthdr header;
	u_char packet[1];
};
#define HPH(hp) (&((hp)->header))
#define HPP(hp) ((hp)->packet)

#define HEADER_PACKET_STACK_POOL_SIZE 100
#define HEADER_PACKET_STACK_PUSH_QUEUE_MAX 10
class cHeaderPacketStack {
public:
private:
	struct sHeaderPacketPool {
		sHeaderPacket *pool[HEADER_PACKET_STACK_POOL_SIZE];
	};
public:
	cHeaderPacketStack(u_int32_t size_max, u_int16_t packet_alloc_size) {
		this->packet_alloc_size = packet_alloc_size;
		pop_queue_size = 0;
		for(int i = 0; i < HEADER_PACKET_STACK_PUSH_QUEUE_MAX; i++) {
			push_queues_size[i] = 0;
		}
		this->stack = new FILE_LINE rqueue_quick<sHeaderPacketPool>(size_max / HEADER_PACKET_STACK_POOL_SIZE, 0, 0, NULL, false, __FILE__, __LINE__);
	}
	~cHeaderPacketStack() {
		for(int ipush = 0; ipush < HEADER_PACKET_STACK_PUSH_QUEUE_MAX; ipush++) {
			for(u_int i = 0; i < push_queues_size[ipush]; i++) {
				delete [] push_queues[ipush].pool[i];
			}
		}
		sHeaderPacket *headerPacket;
		while(pop(&headerPacket) != 2) {
			delete [] headerPacket;
		}
		delete [] headerPacket;
		delete this->stack;
	}
	inline int push(sHeaderPacket *headerPacket, u_int16_t push_queue_index) {
		if(!headerPacket) {
			return(0);
		}
		if(!headerPacket->stack) {
			delete [] headerPacket;
			return(2);
		}
		*(u_char*)&headerPacket->header = 0;
		*headerPacket->packet = 0;
		if(this->push_queues_size[push_queue_index] == HEADER_PACKET_STACK_POOL_SIZE) {
			if(stack->push(&this->push_queues[push_queue_index], false, true)) {
				this->push_queues_size[push_queue_index] = 0;
				//cout << "+" << flush;
			} else {
				delete [] headerPacket;
				return(2);
			}
		}
		if(this->push_queues_size[push_queue_index] < HEADER_PACKET_STACK_POOL_SIZE) {
			this->push_queues[push_queue_index].pool[this->push_queues_size[push_queue_index]] = headerPacket;
			++this->push_queues_size[push_queue_index];
		}
		return(1);
	}
	inline int pop(sHeaderPacket **headerPacket) {
		if(this->pop_queue_size > 0) {
			*headerPacket = this->pop_queue.pool[HEADER_PACKET_STACK_POOL_SIZE - this->pop_queue_size];
			--this->pop_queue_size;
			
		} else {
			if(stack->popq(&this->pop_queue)) {
				*headerPacket = this->pop_queue.pool[0];
				this->pop_queue_size = HEADER_PACKET_STACK_POOL_SIZE - 1;
				//cout << "P" << flush;
			} else {
				*headerPacket = (sHeaderPacket*)new FILE_LINE u_char[sizeof(sHeaderPacket) + packet_alloc_size];
				(*headerPacket)->stack = this;
				(*headerPacket)->packet_alloc_size = packet_alloc_size;
				(*headerPacket)->_detect_headers = false;
				(*headerPacket)->_md5[0] = 0;
				return(2);
			}
		}
		/*
		if(*(u_char*)(&(*headerPacket)->header) ||
		   *(*headerPacket)->packet) {
			printf("bad headerPacket from allocation stack\n");
			abort();
		}
		*/
		(*headerPacket)->_detect_headers = false;
		(*headerPacket)->_md5[0] = 0;
		return(1);
	}
public:
	u_int16_t packet_alloc_size;
	sHeaderPacketPool pop_queue;
	uint16_t pop_queue_size;
	sHeaderPacketPool push_queues[HEADER_PACKET_STACK_PUSH_QUEUE_MAX];
	u_int16_t push_queues_size[HEADER_PACKET_STACK_PUSH_QUEUE_MAX];
	rqueue_quick<sHeaderPacketPool> *stack;
};

inline sHeaderPacket *CREATE_HP(u_int16_t packet_alloc_size) {
	sHeaderPacket *header_packet = (sHeaderPacket*)new FILE_LINE u_char[sizeof(sHeaderPacket) + packet_alloc_size];
	header_packet->stack = NULL;
	header_packet->packet_alloc_size = packet_alloc_size;
	header_packet->_detect_headers = false;
	header_packet->_md5[0] = 0;
	return(header_packet);
}

inline void DESTROY_HP(sHeaderPacket **header_packet) {
	delete [] (u_char*)(*header_packet);
	*header_packet = NULL;
}
inline void PUSH_HP(sHeaderPacket **header_packet, int16_t push_queue_index) {
	if(push_queue_index >= 0 && (*header_packet)->stack) {
		((cHeaderPacketStack*)((*header_packet)->stack))->push(*header_packet, push_queue_index);
		*header_packet = NULL;
	} else {
		DESTROY_HP(header_packet);
	}
}


#endif