//
//  Packet.h
//  Network
//

#ifndef Packet_h
#define Packet_h
#include <stdio.h>
#include <iostream>
#include <vector>
#include <utility>
using namespace std;

class Flow {
public:
    int flowID;
    int srcHost;
    int destHost;
    int srcRouter;
    int destRouter;
    int totalNum;
    // remaining amount
    int remPktNum;
    int curID;
    float tStart;
    
    //data for link rate
    vector<pair<int, int>> avgSizeTime;
    
    Flow(int ID, int src, int dest, int scrR, int destR, int rem, int cur, float t):
    flowID(ID), srcHost(src), destHost(dest), srcRouter(scrR), destRouter(destR), totalNum(rem), remPktNum(rem), curID(cur), tStart(t) {}
};

enum pktType {Data, ACK, CostInfo};

enum nodeType {HOST, ROUTER};

class Node {
public:
    int no;
    nodeType type;
    
    Node() {}
    Node(int noArg, nodeType typeArg): no(noArg), type(typeArg) {}
    Node(const Node &n) {
        no = n.no;
        type = n.type;
    }
};
class Packet {
public:
    int ID;
    // ACK or DATA or CostInfo
    pktType type;
    // start time of generating and arriving time at dest
    float tStart;
    float tEnd;
    // src and dest
    Node srcNode;
    int srcRouter;
    Node destNode;
    int destRouter;
    int me;
    // the time enters a link and the time leaves a link
    float tEnter;
    float tLeave;
    
    // constructor
    Packet(int IDArg, pktType typeArg, float tStartArg, float tEnterArg, int meArg);
    virtual ~Packet();
};

class DataPkt : public Packet {
public:
    // timeOutOn is set to be true when a dataPkt is sent
    // timeOutOn is reset if ACK has been received or 'not receive' event has been triggured
    bool timeOutOn;
    int version;
    Flow *f;
    
    // constructor
    DataPkt(int IDArg, float tStartArg, float tEnterArg, int meArg, int v, Flow *fArg);
    virtual ~DataPkt();
};

class CostPkt : public Packet {
public:
    int linkNo;
    int me;
    bool timeOutOn;
    int version;
    vector<float> costInfo;
    
    // constructor
    CostPkt(int IDArg, float tStartArg, float tEnterArg, int linkNoArg, int meArg, int scrNoArg, int destNoArg, int v, vector<float> costInfoArg);
    virtual ~CostPkt();
};

class ACKPkt : public Packet {
public:
    DataPkt * dp;
    CostPkt * cp;
    
    // constructor
    ACKPkt(int IDArg, float tStartArg, float tEnterArg, int meArg, DataPkt * dpArg = NULL, CostPkt *cp = NULL);
    virtual ~ACKPkt();
};

#endif /* Packet_h */
