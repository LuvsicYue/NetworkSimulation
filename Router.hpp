//
//  Router.hpp
//  Network
//

#ifndef Router_h
#define Router_h

#include "Link.hpp"
#include <stdio.h>
#include <iostream>
#include <utility>
#include <vector>
#include <queue>
#include <cmath>


using namespace std;

extern const int DATA_SIZE;
extern const int ACK_SIZE;

class Routing {
public:
    float cost;
    int nextHop;
    int portNo;
    
    Routing(float costArg, int nextHopArg, int portNoArg): cost(costArg), nextHop(nextHopArg), portNo(portNoArg) {}  // Constructor
};

class Port {
public:
    Node n;
    Link *l;

    int me;
    
    Port() {}  // Constructor
    Port(Node nArg, Link *lArg, int meArg): n(nArg), l(lArg), me(meArg) {}  // Constructor with argu
    Port(const Port &p) {  // Constructor with Port
        n = p.n;
        l = p.l;
        me = p.me;
    }
};

class Router {
public:
    int routerID;
    vector<Routing> routingTable;
    vector<float> costs;
    vector<float> tempCosts;
    vector<Port> ports;
    vector<int> neighbors;
    vector<float> nbCosts;

    int version;
    bool initial;
    
    Router(int ID, vector<Port> ports);  // Constructor
    Packet * send(pktType type, float time, int portNo = 0, CostPkt * cp = NULL);  // Send ACK or CostInfo, return the pointer of pkt
    int forward(Packet * p, float t);
    void computeCost();
    bool updateRT(CostPkt * cp);
    bool isNeighbor(int i);
    int getNbIndex(int no);
    int getPortNo(Node &i);
    void print();
};

#endif /* Router_h */
