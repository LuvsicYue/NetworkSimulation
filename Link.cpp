//
//  Link.cpp
//  Network
//

#include "Link.hpp"

Link::Link(int lID, float lrate, int ldelay, int lbuffer_size, Node ln1, Node ln2):
linkID(lID), rate(lrate), delay(ldelay), buffer_size(lbuffer_size) {
    ns[0] = ln1;
    ns[1] = ln2;
    onTheWay = 0;
    workings[0] = false;
    workings[1] = false;
    cost = 0;
    tempCost = 0;
    curDest = 1;
    curRate = 0.0;
    buffer_caps[0] = 0;
    buffer_caps[1] = 0;
}

Packet * Link::send(int me) {
    Packet *p = NULL;
    if (workings[me]) return p;
    if ((me == 0 && ! buffer1.empty()) || (me == 1 && !buffer2.empty()) ) {
        if (me == 0) {
            p = buffer1.front();
            buffer1.pop();
        }
        else if (me == 1) {
            p = buffer2.front();
            buffer2.pop();
        }
        int size = (p->type == ACK) ? ACK_SIZE : DATA_SIZE;
        buffer_caps[me] -= size;
        buffer_caps[me] = (buffer_caps[me] >= 0) ? buffer_caps[me] : 0;
    }
    if (p != NULL) workings[me] = true;
    return p;
}