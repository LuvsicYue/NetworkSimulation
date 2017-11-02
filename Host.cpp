//
//  Host.cpp
//  Network
//  



#include "Host.hpp"
#include <iostream>
float scale = 0.2;
extern bool MODE;
extern vector<FILE *> PacketLossFiles;
extern vector<string> PacketLossNames;
// constructor
Host::Host() {}
Host::Host(int ID, Flow * fArg, Link * lArg, int meArg): hostID(ID), f(fArg), l(lArg), me(meArg)  {
    tOut = 300.0;
    RTT = 100.0;
    lastTimeOut = 0.0;
    baseRTT = INFINITY;
    lastSeqBeforeHandle = 0;
    timeOutHappenedInFR = false;
    ssthresh = INFINITY;
    status = "ss";
    phase = "Normal";
    wndSize = 1.0;
    // the first pkt I want is 1
    want = 1;
    wantVersion = 1;
    numsDupACK = 0;
    routerID = l->ns[1-me].no;
    FRtirgSeq.first = 0;
    FRtirgSeq.second = 1;
}

void Host::setTOutRTT(float time, ACKPkt * ackp) {
    float tTrans = ackp->dp->tStart;
    RTT = RTT*scale + (time-tTrans)*(1-scale);
    tOut = RTT*3;
    baseRTT = (RTT < baseRTT) ? RTT:baseRTT;
}
// congestion control
void Host::cControl(string state) {
    if (state == "Received in Normal") {
        if (MODE == false) return;
        wndSize = (status == "ss")? wndSize + 1 : wndSize + 1.0/wndSize;
        if (wndSize > ssthresh) {
            status = "ac";
        }
    }
    else if (state == "Received in FR") {
        wndSize++;
    }
    else if (state == "Exit FR without TO") {
        wndSize = ssthresh;
    }
    else if (state == "3Dup") {
        ssthresh = max(wndSize/2.0, 1.0);
        wndSize = ssthresh + 3;
    }
    else if (state == "TimeOut") {
        ssthresh = max(wndSize/2.0, 1.0);
        wndSize = 1;
        status = "ss";
    }
}
void retransmit();

/* generate a new pkt, -- flow, push into the link buffer
 * return the pkt I generated
 */
Packet * Host::send(pktType type, float time, int seqNo, int v, DataPkt * dpArg) {
    Packet *p = nullptr;
    int size = DATA_SIZE;
    if (type == ACK) {
        p = new ACKPkt(want, time, time, me, dpArg);
        size = ACK_SIZE;
    }
    else if (type == Data) {
        size = DATA_SIZE;
        if (seqNo == 0) {
            p = new DataPkt(f->curID, time, time, me, v, f);
            f->curID++;
            f->remPktNum --;
        }
        else
            p = new DataPkt(seqNo, time, time, me, v, f);
    }
    if (l->buffer_caps[me] + size <= l->buffer_size) {
        if (me == 0) l->buffer1.push(p);
        else if (me == 1) l->buffer2.push(p);
        l->buffer_caps[me] += size;
    }
    else {
        cout<<p->ID<<" is losted at host 0"<<endl;
        PacketLossFiles[l->linkID] = fopen (PacketLossNames[l->linkID].c_str(), "a");
        fprintf(PacketLossFiles[l->linkID] , "%-10.2f %-10d\n", time, 1);
        fclose(PacketLossFiles[l->linkID] );
    }
    if (p == nullptr) {
        cout << "FATAL!!!!!!!!!!" << endl;
        exit(-1);
    }
    return p;
}