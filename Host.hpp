//
//  Host.hpp
//  Network
//

#ifndef Host_h
#define Host_h
#include "Packet.hpp"
#include "Link.hpp"
#include <math.h>
#include <string>
#include <queue>
#include <algorithm>
using namespace std;

extern const int DATA_SIZE;
extern const int ACK_SIZE;

class Host{
public:
    int hostID;
    
    float ssthresh;
    float RTT;
    float tOut;
    float baseRTT;
    int lastSeqBeforeHandle;
    float lastTimeOut;
    // ss or ac
    string status;
    // FR or Normal
    string phase;
    pair<int, int> FRtirgSeq;
    vector<int> TOtrigSeq;
    bool timeOutHappenedInFR;
    
    float wndSize;
    // one for seqNo the other for version
    vector<pair<int,int>> wnd;
    
    // if 1 is lost, want will be 1 and 2,3,4,5 (received) will be pushed into receiver
    int want;
    int wantVersion;
    priority_queue<int, vector<int>, greater<int>> receiver;
    
    // the current processing flow ID.
    Flow *f;
    
    Link *l;
    int me;
    int routerID;
    
    // the number of duplicate ACK received.
    int numsDupACK;
    
    // constructor
    Host();
    Host(int ID, Flow * fArg, Link * lArg, int meArg);
    // update the tOut
    void setTOutRTT(float time, ACKPkt * ackp);
    // congestion control
    void cControl(string state);
    void retransmit();
    // generate a new pkt
    // return the pkt I generated
    Packet * send(pktType type, float time, int seqNo, int v, DataPkt * dpArg = NULL);
};

#endif /* Host_h */
