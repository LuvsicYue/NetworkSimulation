//
//  Router.cpp
//  Network
//

#include "Router.hpp"

extern vector<FILE *> PacketLossFiles;
extern vector<string> PacketLossNames;

Router::Router(int ID, vector<Port> portsArg): routerID(ID), ports(portsArg) {
    initial = true;
    version = 1;
    for (vector<Port>::iterator ip = ports.begin(); ip != ports.end(); ip++) {
        if (ip->n.type == ROUTER) {
            neighbors.push_back(ip->n.no);
            nbCosts.push_back(ip->l->delay * ip->l->rate);
        }
    }
}

bool Router::isNeighbor(int i) {
    for (int j = 0; j <= neighbors.size() - 1; j++) {
        if (i == neighbors[j]) {
            return true;
        }
    }
    return false;
}

int Router::getNbIndex(int no) {
    int res = -1;
    for (int i = 0; i <= neighbors.size() - 1; i++) {
        if (no == neighbors[i]) {
            return i;
        }
    }
    return res;
}

int Router::getPortNo(Node &i) {
    for (int j = 0; j <= ports.size() - 1; j++) {
        if (ports[j].n.no == i.no && ports[j].n.type == i.type) {
            return j;
        }
    }
    return -1;
}

Packet * Router::send(pktType type, float time,  int portNo, CostPkt * cp) {
    Packet *p = nullptr;
    Port pt;
    int size = DATA_SIZE;
     
    if (cp != NULL) {  // Case not NULL
        int num = (type == ACK) ? getPortNo(cp->srcNode) : getPortNo(cp->destNode);
        if (num == -1) {
            return NULL;
        }
        pt = ports[num];
    } else {
        pt = ports[portNo];
    }
    
    if (type == ACK) {
        p = new ACKPkt(cp->ID, time, time, pt.me, NULL, cp);
        size = ACK_SIZE;
    } else if (type == CostInfo) {
        int id = (cp == NULL) ? 1 : cp->ID+1;
        for (int i = 0; i <= routingTable.size() - 1; i++) {
            if (routingTable[i].nextHop == pt.n.no && i != pt.n.no) {
                tempCosts[i] = INFINITY;
            } else {
                tempCosts[i] = costs[i];
            }
        }
        p = new CostPkt(id, time, time, pt.l->linkID, pt.me, routerID, pt.n.no, version, tempCosts);
        size = DATA_SIZE;
    }
    if (pt.l->buffer_caps[pt.me] + size <= pt.l->buffer_size) {
        if (pt.me == 0) {
            pt.l->buffer1.push(p);
        }
        else if (pt.me == 1) {
            pt.l->buffer2.push(p);
        }
        pt.l->buffer_caps[pt.me] += size;
    } else {
        cout<<"router loss "<< p->type<<" p from "<<p->srcNode.no <<" to "<<p->destNode.no<<" is lost\n";
    }
    return p;
}

void Router::computeCost() {
    for (int i = 0; i <= ports.size() - 1; i++) {
        if (ports[i].n.type == ROUTER) {
            
            // Update the neighbors first, if A is the neighbor, then set no = A
            // First update B->A->...->A
            int no = ports[i].n.no;
            float newCost = ports[i].l->cost + ports[i].l->delay * ports[i].l->rate;
            float deltCost = newCost - nbCosts[getNbIndex(no)];
            nbCosts[getNbIndex(no)] = newCost;
            for (int j = 0; j <= routingTable.size() - 1; j++) {
                if (routingTable[j].nextHop == no) {
                    routingTable[j].cost += deltCost;
                }
            }
        }
    }
    
    for (int i = 0; i <= routingTable.size() - 1; i++) {  // Case for A->B->C change to A->C
        if (isNeighbor(i) && routingTable[i].nextHop != i) {
            if (routingTable[i].cost > nbCosts[getNbIndex(i)]) {
                routingTable[i].nextHop = i;
                routingTable[i].cost = nbCosts[getNbIndex(i)];
                Node n(i, ROUTER);
                routingTable[i].portNo = getPortNo(n);
            }
        }
    }
    for (int i = 0; i <= costs.size() - 1; i++) {
        costs[i] = routingTable[i].cost;
    }
}

int Router::forward(Packet * p, float t) {
    int portNo = 0;
    if (p->destRouter == routerID) {
        portNo = getPortNo(p->destNode);
    } else {
        int no = p->destRouter;
        portNo = routingTable[no].portNo;
    }
    int me = ports[portNo].me;
    Link* lk = ports[portNo].l;
    p->me = me;
    int size = (p->type == Data) ? DATA_SIZE : ACK_SIZE;
    if (lk->buffer_caps[me] + size <= lk->buffer_size) {
        if (me == 0) lk->buffer1.push(p);
        else if (me == 1) lk->buffer2.push(p);
        lk->buffer_caps[me] += size;
    } else {
        cout<<"router "<< routerID <<" lose "<< p->ID<<" p from "<<p->srcNode.no<<" at time"<< t<<endl;
        PacketLossFiles[lk->linkID] = fopen (PacketLossNames[lk->linkID].c_str(), "a");
        fprintf(PacketLossFiles[lk->linkID] , "%-10.2f %-10d\n", t, 1);
        fclose(PacketLossFiles[lk->linkID] );
    }
    return lk->linkID;
}

// If me = A, you = B, B must be the neightbor of A, set B in the nextHop
// Find all entries with nextHop B, if A->B->......->C 's cost changes, 
// then find all entries with nextHop C, enqueue.
// Change the cost of A->C->......->other && compare A->B->...->other
// Terminate when queue empty
bool Router::updateRT(CostPkt * cp) {
    bool changed = false;
    int you = cp->srcRouter;  // RouterNo sends me cost Info
    float cost_me_you = cp->costInfo[routerID];  // Cost between me and the router
    Node n(you, ROUTER);
    int pt = getPortNo(n);
    if (pt == -1) return false;
    // Update
    queue<pair<int, float>> q;

    for (int i = 0; i <= routingTable.size() - 1; i++) {  // Find entries with nextHop B
        float deltCost = 0;
        if (routingTable[i].nextHop == you) {
            float newCost = cost_me_you + cp->costInfo[i];
            deltCost = newCost - routingTable[i].cost;
            if (fabsf(deltCost - 0) < 1e-7) continue;
            routingTable[i].cost = newCost;
            changed = true;
        }
    }
    
    for (int i = 0; i <= routingTable.size() - 1; i++) {  // NextHop not B
        if (routingTable[i].nextHop != you) {
            float cost = cost_me_you + cp->costInfo[i];
            if (routingTable[i].cost > cost) {
                routingTable[i].portNo = pt;
                routingTable[i].cost = cost;
                routingTable[i].nextHop = you;
                changed = true;
            }
        }
    }
    
    if (changed) nbCosts[getNbIndex(you)] = cost_me_you;
    
    for (int i = 0; i <= routingTable.size() - 1; i++) {  // Update costs
        costs[i] = routingTable[i].cost;
    }
    return changed;
}

void Router::print() {
    cout<<routerID<<"\'s routing table\n";
    cout<<"dest    "<<"cost    "<<"nextHop    "<<endl;
    for (vector<Routing>::iterator ir = routingTable.begin(); ir != routingTable.end(); ir++) {
        cout<<ir-routingTable.begin()<<"    "<<ir->cost<<"     "<<ir->nextHop<<endl;
    }
}
