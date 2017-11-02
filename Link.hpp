//
//  Link.hpp
//  Network
//

#ifndef Link_h
#define Link_h
#include <stdio.h>
#include <queue>
#include <vector>
#include "Packet.hpp"

extern const int DATA_SIZE;
extern const int ACK_SIZE;

class Link {
public:
    int linkID;
    // working is true if the node is "transmit" the pkt.
    bool workings[2];
    // the num of pkts on the link
    int onTheWay;
    // temp cost & cost
    float cost;
    float tempCost;
    // point to nodes at the ends
    Node ns[2];
    float rate;
    float curRate;
    int delay;
    int buffer_size;
    /* the end of the current sending pkt
     * a pkt from left(1) to right(2) of the link
     * then dest=n2, otherwise dest = n1;
     */
    int curDest;
    
    int buffer_caps[2];
    queue<Packet *> buffer1;
    queue<Packet *> buffer2;
    
    //constructor
    Link(int lID, float lrate, int ldelay, int lbuffer_size, Node ln1, Node ln2);
    /* check whether buffer1 and buffer2 have pkt.
     * check which pkt arrived earlier
     * if buffer1 ealier, buffer1.dequeue(), dest = n2;
     * else buffer2.dequeue(), dest = n1;
     * return the pktNo sent
     */
    Packet * send(int me);
};

#endif /* Link_h */
