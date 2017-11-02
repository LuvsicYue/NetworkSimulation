//
//  Packet.cpp
//  Network
//

#include "Packet.hpp"


/////////////////////////////////////////////////////////////////
Packet::Packet(int IDArg, pktType typeArg, float tStartArg,
               float tEnterArg, int meArg) : ID(IDArg), type(typeArg), tStart(tStartArg), tEnter(tEnterArg), me(meArg) {}

Packet::~Packet() {}
/////////////////////////////////////////////////////////////////
DataPkt::DataPkt(int IDArg, float tStartArg, float tEnterArg, int meArg, int v,
                 Flow *fArg): Packet(IDArg, Data, tStartArg, tEnterArg, meArg), timeOutOn(true), version(v), f(fArg) {
    srcNode.no = f->srcHost;
    srcNode.type = HOST;
    destNode.no = f->destHost;
    destNode.type = HOST;
    srcRouter = f->srcRouter;
    destRouter = f->destRouter;
}
DataPkt::~DataPkt() {
}

/////////////////////////////////////////////////////////////////
CostPkt::CostPkt(int IDArg, float tStartArg, float tEnterArg, int linkNoArg, int meArg, int scrNoArg, int destNoArg,  int v, vector<float> costInfoArg):
Packet(IDArg, CostInfo, tStartArg, tEnterArg, meArg), linkNo(linkNoArg), timeOutOn(true), version(v), costInfo(costInfoArg) {
    srcNode.no = scrNoArg;
    srcNode.type = ROUTER;
    destNode.no = destNoArg;
    destNode.type = ROUTER;
    srcRouter = scrNoArg;
    destRouter = destNoArg;
}
CostPkt::~CostPkt() {}

/////////////////////////////////////////////////////////////////
ACKPkt::ACKPkt(int IDArg, float tStartArg, float tEnterArg, int meArg, DataPkt * dpArg, CostPkt * cpArg): Packet(IDArg, ACK, tStartArg, tEnterArg, meArg), dp(dpArg), cp(cpArg) {
    if (dpArg != NULL && cpArg == NULL) {
        srcNode.no = dp->f->destHost;
        srcNode.type = HOST;
        destNode.no = dp->f->srcHost;
        destNode.type = HOST;
        srcRouter = dp->f->destRouter;
        destRouter = dp->f->srcRouter;
    }
    else if (dpArg == NULL && cpArg != NULL) {
        srcNode.no = cp->destNode.no;
        srcNode.type = ROUTER;
        srcRouter = srcNode.no;
        destNode.no = cp->srcNode.no;
        destNode.type = ROUTER;
        destRouter = destNode.no;
    }
}
ACKPkt::~ACKPkt() {}