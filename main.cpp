//
//  main.cpp
//  Network
//
// Host with initial timeOUt to be 300 ms, if 1 is lost, then the timeOut will be
// computed by ack.
// Assume that the front in wnd must be the lost one.
//
// Router with timeout to be 200ms, router's ACK's ID is the portNo
//
//
#include <fstream>
#include <iostream>
#include <sstream>
#include <math.h>
#include <queue>
#include "Host.hpp"
#include "Router.hpp"
#include "Packet.hpp"
#include "Link.hpp"

////////////////////////// Global Varibal /////////////////////////
vector<Host *> hosts;
vector<Router *> routers;
vector<Packet *> pkts;
vector<Flow *> flows;
vector<Link *> links;
vector<Packet *> TOpkts;

vector<Node *> nodes;
vector<Port *> ports;

// ms
static float CUR_TIME;
static float TIME_OUT = 200.0;
const float UPDATE_TIME = 10000.0;
const float LCostUpdate_TIME = 200.0;
// bits
const int DATA_SIZE = 1024*8;
const int ACK_SIZE = 64*8;
// changed to record whether routers have been initialized
vector<int> changes;
// finishes[i] = 0 means the ith flow finishes its transportation.
vector<int> finishes;
// scale
float scale1 = 0.01;
// fast or reno, MODE = true, reno; else fast;
bool MODE = true;
float gama = 0.1;
float alpha = 20;
////////////////////////////// LOGGER /////////////////////////////
// file recording link rate
vector<FILE *> LinkRateFiles;
vector<string> LinkRateFileNames;
vector<FILE *> FlowRateFiles;
vector<string> FlowRateFileNames;
vector<FILE *> WindSizeFiles;
vector<string> WindSizeFileNames;
vector<FILE *> BufferFiles;
vector<string> BufferFileNames;
vector<FILE *> PacketLossFiles;
vector<string> PacketLossNames;
vector<FILE *> PacketDelayFiles;
vector<string> PacketDelayNames;
/////////////////////////// HELPER FUNCTION ///////////////////////
float getFlowRate(Flow * f) {
    float totalAmount = 0.0;
    float totalTime = 0.0;
    for (vector<pair<int, int>>::iterator ia = f->avgSizeTime.begin(); ia != f->avgSizeTime.end(); ia++) {
        totalAmount += get<0>(*ia)*1.0;
        totalTime += get<1>(*ia)*1.0;
    }
    return totalAmount/totalTime;
}
bool allUnchanged() {
    int sum = 0;
    for (int i = 0; i < (int)changes.size(); ++i) {
        sum += changes[i];
    }
    return (sum == 0);
}
////////////////////////// EVEN CLASSES////////////////////////////

class Event {
public:
    //specify the number of objects
    int no;
    //specify the happening time
    float t;
    int n;
    
    Event();
    Event(int noArg, float tArg, int nArg): no(noArg), t(tArg), n(nArg){}
    virtual void action()=0;
    virtual ~Event() {}
};

////////////////////////// Global Varibal /////////////////////////
class comparator {
public:
    bool operator() (const Event * e1, const Event * e2) const {
        return (e1->t > e2->t);
    }
};
// priority queue
priority_queue<Event *, vector<Event *>, comparator> PQ;

////////////////////////// Derived Classes /////////////////////////
class RouterTimeOut : public Event {
public:
    Packet * p;
    // constructor
    RouterTimeOut(int no, float t, Packet *pArg): Event(no, t, 0), p(pArg) {}
    
    virtual void action();
    virtual ~RouterTimeOut() {}
};

class RouterReceive : public Event{
public:
    Packet * p;
    
    // constructor
    RouterReceive(int no, float t, Packet * pArg): Event(no, t, 100), p(pArg) {}
    
    virtual void action();
    virtual ~RouterReceive() {}
};

class RouterUpdate : public Event{
public:
    //constructor
    RouterUpdate(int no, float t): Event(no, t, 200) {};
    
    //call function send cost info to neighbor routers
    void action();
    virtual ~RouterUpdate() {}
};

class HostTimeOut : public Event {
public:
    Packet * p;
    // constructor
    HostTimeOut(int no, float t, Packet *pArg): Event(no, t, pArg->ID), p(pArg) {}
    
    virtual void action();
    virtual ~HostTimeOut() {}
};

class HostReceive : public Event {
public:
    Packet * p;
    //constructor
    HostReceive(int no, float t, Packet * pArg): Event(no, t, 400), p(pArg) {}
    
    virtual void action();
    virtual ~HostReceive() {}
};

class HostFastUpdate : public Event {
public:
    //constructor
    HostFastUpdate(int no, float t) : Event(no, t, 500) {}
    
    virtual void action();
    virtual ~HostFastUpdate() {}
};

class LinkCostUpdate : public Event {
public:
    // constructor
    LinkCostUpdate(int no, float t) : Event(no, t, 600) {}
    
    virtual void action();
    virtual ~LinkCostUpdate() {}
};
class LinkAvailable : public Event {
public:
    int direction;
    // end flag, indicating that this event is triggeredd
    // because one pkt arrives at the end.
    bool pktArrivesAtEnd;
    // pkt received
    Packet *arrP;
    
    //constructor
    LinkAvailable(int no, float t, int direArg, bool f, Packet *p): Event(no, t, 700), direction(direArg), pktArrivesAtEnd(f), arrP(p) {}
    
    virtual void action();
    virtual ~LinkAvailable() {}
};

/////////////////////// HELPER FUNCTIONS ///////////////////////
void activateLink(Link *l, int me) {
    Packet *p = l->send(me);
    if (p == NULL) return;
    BufferFiles[l->linkID] = fopen (BufferFileNames[l->linkID].c_str(), "a");
    fprintf(BufferFiles[l->linkID] , "%-10.2f %-10lu %-10lu\n", CUR_TIME, l->buffer1.size(), l->buffer2.size());
    fclose(BufferFiles[l->linkID] );
    l->workings[me] = true;
    float t = (p->type != ACK) ? DATA_SIZE/l->rate : ACK_SIZE/l->rate;
    LinkAvailable *la1 = new LinkAvailable(l->linkID, CUR_TIME + t, 1-me, false, NULL);
    LinkAvailable *la2 = new LinkAvailable(l->linkID, CUR_TIME + l->delay + t, 1-me, true, p);
    PQ.push(la1);
    PQ.push(la2);
}
/////////////////////// IMPLEMENTATIONS ///////////////////////
void RouterTimeOut::action() {
    Router *r = routers[no];
    CostPkt *cp = dynamic_cast<CostPkt *>(p);
    if (cp->timeOutOn == true) {
        // cp != NULL, we set the portNo = 0
        Packet *reP = r->send(CostInfo, CUR_TIME, 0, cp);
        pkts.push_back(reP);
        RouterTimeOut *hto = new RouterTimeOut(no, CUR_TIME+TIME_OUT, reP);
        PQ.push(hto);
        // activate the not-busy link
        Link *l = links[cp->linkNo];
        activateLink(l, reP->me);
    }
    //delete p;
}

void RouterReceive::action() {
    Router * r =  routers[no];
    bool changed;
    if (p->type == CostInfo) {
        CostPkt * cp = dynamic_cast<CostPkt *>(p);
        cout<<"////////////////////\n";
        r->print();
        changed = r->updateRT(cp);
        r->print();
        // portNo = 0 beacuse we have cp and will use cp
        Packet * newPkt = r->send(ACK, CUR_TIME, 0, cp);
        pkts.push_back(newPkt);
        // activate the not-busy link
        Link *l = links[cp->linkNo];
        activateLink(l, newPkt->me);
        
        changes[no] = (changed) ? 1:0;
        r->initial = (allUnchanged()) ? false: r->initial;
        if (changed) {
            r->version++;
            for (int i = 0; i < r->ports.size(); i++) {
                if (r->ports[i].n.type == ROUTER) {
                    // portNo is i and cp = NULL
                    Packet * newPkt = r->send(CostInfo, CUR_TIME+1, i, NULL);
                    pkts.push_back(newPkt);
                    RouterTimeOut *rto = new RouterTimeOut(no, CUR_TIME + 1 + TIME_OUT, newPkt);
                    PQ.push(rto);
                    // activate the not-busy link
                    Link *l = r->ports[i].l;
                    activateLink(l, newPkt->me);
                }
            }
        }
        
    }
    else if (p->type == Data || (p->type == ACK && p->destNode.type == HOST)) {
        int lNo = r->forward(p, CUR_TIME);
        Link *l = links[lNo];
        activateLink(l, p->me);
    }
    else if (p->type == ACK && p->destNode.type == ROUTER) {
        ACKPkt * ack = dynamic_cast<ACKPkt *>(p);
        ack->cp->timeOutOn = false;
        // delete p;
    }
}

void RouterUpdate::action() {
    Router * r = routers[no];
    int sum = 0;
    for (int i = 0; i < finishes.size(); ++i) {
        sum += finishes[i];
    }
    if (sum == 0)
        return;
    r->version++;
    if (no == 0) {
        for (int i = 0; i < links.size(); ++i) {
            links[i]->cost = links[i]->tempCost/(UPDATE_TIME / LCostUpdate_TIME);
            links[i]->tempCost = 0;
        }
        for (int i = 0; i < routers.size(); ++i) {
            changes[i] = 1;
        }
    }
    cout<<"//////////////////\n";
    r->print();
    r->computeCost();
    
    r->print();
    for (int i = 0; i < r->ports.size(); i++) {
        if (r->ports[i].n.type == HOST) continue;
        // portNo is i and cp = NULL
        Packet * newPkt = r->send(CostInfo, CUR_TIME, i, NULL);
        pkts.push_back(newPkt);
        RouterTimeOut *rto = new RouterTimeOut(no, CUR_TIME + TIME_OUT, newPkt);
        PQ.push(rto);
        // activate the not-busy link
        Link *l = r->ports[i].l;
        activateLink(l, newPkt->me);
    }
    RouterUpdate *ru = new RouterUpdate(no, CUR_TIME + UPDATE_TIME);
    PQ.push(ru);
}

void HostTimeOut::action() {
    Host * h = hosts[no];
    DataPkt *data = dynamic_cast<DataPkt *>(p);
    bool FRrep = false;
    if (data->timeOutOn && !(data->ID < h->FRtirgSeq.first || (data->ID == h->FRtirgSeq.first && data->version <= h->FRtirgSeq.second))) {
        data->timeOutOn = false;
        if (h->phase == "FR") {
            // if (data->ID > h->lastSeqBeforeHandle)
            /*      h->timeOutHappenedInFR = true;
             if (h->FRtirgSeq.first != data->ID) {
             FRrep = true;
             h->TOtrigSeq.push_back(data->ID);
             }*/
            h->cControl("TimeOut");
            h->phase = "Normal";
            // record
            WindSizeFiles[h->hostID] = fopen (WindSizeFileNames[h->f->flowID].c_str(), "a");
            fprintf(WindSizeFiles[h->hostID] , "%-10.2f %-10.2f\n", CUR_TIME, h->wndSize);
            fclose(WindSizeFiles[h->hostID] );
            h->lastSeqBeforeHandle = h->wnd[h->wnd.size()-1].first;
        }
        else if (h->phase == "Normal") {
            if (data->ID > h->lastSeqBeforeHandle) {
                h->cControl("TimeOut");
                // record
                WindSizeFiles[h->hostID] = fopen (WindSizeFileNames[h->f->flowID].c_str(), "a");
                fprintf(WindSizeFiles[h->hostID] , "%-10.2f %-10.2f\n", CUR_TIME, h->wndSize);
                fclose(WindSizeFiles[h->hostID] );
                h->lastSeqBeforeHandle = h->wnd[h->wnd.size()-1].first;
            }
        }
        if (FRrep || h->phase == "Normal") {
            h->lastTimeOut = CUR_TIME;
            // retransmit needed.
            Packet *reP = h->send(Data, CUR_TIME, data->ID, data->version+1);
            pkts.push_back(reP);
            HostTimeOut *hto = new HostTimeOut(no, CUR_TIME+h->tOut, reP);
            PQ.push(hto);
            // set the receiver's version
            hosts[reP->destNode.no]->wantVersion = data->version+1;
            // change the version of the corresponding data in wnd
            for (int i = 0; i < h->wnd.size(); ++i) {
                if (h->wnd[i].first == data->ID) {
                    h->wnd[i].second = data->version+1;
                    break;
                }
            }
            h->wnd.front().second = data->version+1;
            // activate the not-busy link
            activateLink(h->l, reP->me);
            //  delete p;
        }
    }
    else {
        TOpkts.push_back(data);
    }
}

void HostReceive::action() {
    Host * h = hosts[no];
    if (p == NULL) {
        int i = 1;
        while (h->wnd.size() < floor(h->wndSize) && h->f->remPktNum > 0) {
            // generate a new pkt by calling send(); not retransmit seqNo = 0, version = 1;
            Packet *newP = h->send(Data, CUR_TIME + scale1*i, 0, 1);
            // push the new generated pkt into vector pkts
            pkts.push_back(newP);
            // push into the wnd with version 1
            pair<int, int> pktPair(newP->ID, 1);
            h->wnd.push_back(pktPair);
            // a new PktTimeOut event should be set
            HostTimeOut *hto = new HostTimeOut(no, CUR_TIME+h->tOut + scale1*i, newP);
            PQ.push(hto);
            // activate the not-busy link
            Link *l = h->l;
            activateLink(l, newP->me);
            i++;
        }
        return;
    }
    // if the dest doesn't match, return
    if (p->destNode.no != no) return;
    p->tEnd = CUR_TIME;
    if (p->type == ACK) {
        ACKPkt *ack = dynamic_cast<ACKPkt *>(p);
        // update the timeout
        h->setTOutRTT(CUR_TIME, ack);
        if (ack->ID == h->f->totalNum + 1)
            finishes[h->f->flowID] = 0;
        if (h->phase == "Normal") {
            if (ack->dp->ID > h->lastSeqBeforeHandle && ack->dp->version == 1 && ack->dp->tStart > h->lastTimeOut) {
                if (ack->ID > 1 && ack->ID < 5 && MODE == false) {
                    HostFastUpdate *hfu = new HostFastUpdate(h->f->srcHost, CUR_TIME + 1);
                    PQ.push(hfu);
                }
                // adjust the wndsize after receiving an ACK
                h->cControl("Received in Normal");
                // record
                WindSizeFiles[h->hostID] = fopen (WindSizeFileNames[h->f->flowID].c_str(), "a");
                fprintf(WindSizeFiles[h->hostID] , "%-10.2f %-10.2f\n", CUR_TIME, h->wndSize);
                fclose(WindSizeFiles[h->hostID] );
            }
            // this ack is what we want
            if (ack->ID > get<0>(h->wnd.front()) ) {
                // set the timeOutOn as false so that it will not triggure the timeOut event
                ack->dp->timeOutOn = false;
                // if the version no doesn't match, but the first one is the retransmit one, ignore this ack.
                if (ack->dp->ID == get<0>(h->wnd.front()) && ack->dp->version != get<1>(h->wnd.front())) return;
                // (1), 2,3,4,5 Then after 1 retrasmit, ack will be 6, so 1~5 will be poped out
                while (!h->wnd.empty() && ack->ID > get<0>(h->wnd.front())) {
                    h->wnd.erase(h->wnd.begin());
                }
                h->numsDupACK = 0;
            }
            // while num of ACK not received is smaller than wndSize && remPktNum > 0, we send
            int i = 1;
            while (h->wnd.size() < floor(h->wndSize) && h->f->remPktNum > 0 && i < 3) {
                // generate a new pkt by calling send(); not retransmit seqNo = 0, version = 1;
                Packet *newP = h->send(Data, CUR_TIME+scale1*i, 0, 1);
                // push the new generated pkt into vector pkts
                pkts.push_back(newP);
                // push into the wnd with version 1
                pair<int, int> pktPair(newP->ID, 1);
                h->wnd.push_back(pktPair);
                // a new PktTimeOut event should be set
                HostTimeOut *hto = new HostTimeOut(no, CUR_TIME+h->tOut + scale1*i, newP);
                PQ.push(hto);
                // activate the not-busy link
                activateLink(h->l, newP->me);
                i++;
            }
            // this ack is not we want
            if (ack->ID <= get<0>(h->wnd.front())) {//
                // turn off the timeOut of the corresponding pkt
                ack->dp->timeOutOn = false;
                if (h->status == "ss") return;
                if (ack->ID < h->wnd.front().first) {
                    //   delete p;
                    return;
                }
                h->numsDupACK++;
                // loss detected
                if (h->numsDupACK == 3) {
                    for (int i = 0; i < h->TOtrigSeq.size(); ++i) {
                        // if found, means this pkt has gone through TO and has been retransmitted.
                        if (h->TOtrigSeq[i] == ack->ID) {
                            h->TOtrigSeq.erase(h->TOtrigSeq.begin() + i);
                            //  delete  p;
                            return;
                        }
                    }
                    h->phase = "FR";
                    if (ack->ID > h->lastSeqBeforeHandle) {
                        h->cControl("3Dup");
                        // record
                        WindSizeFiles[h->hostID] = fopen (WindSizeFileNames[h->f->flowID].c_str(), "a");
                        fprintf(WindSizeFiles[h->hostID] , "%-10.2f %-10.2f\n", CUR_TIME, h->wndSize);
                        fclose(WindSizeFiles[h->hostID] );
                        h->lastSeqBeforeHandle = h->wnd[h->wnd.size()-1].first;
                    }
                    
                    if (h->FRtirgSeq.first == ack->ID) {
                        h->FRtirgSeq.second++;
                    }
                    else {
                        h->FRtirgSeq.first = ack->ID;
                        h->FRtirgSeq.second = 1;
                    }
                    h->numsDupACK = 0;
                    // retransmit
                    Packet *reP = h->send(Data, CUR_TIME, ack->ID, h->FRtirgSeq.second+1);
                    // set the receiver's version
                    hosts[reP->destNode.no]->wantVersion = ack->dp->version+1;
                    pkts.push_back(reP);
                    HostTimeOut *hto = new HostTimeOut(no, CUR_TIME+h->tOut, reP);
                    PQ.push(hto);
                    // change the version of the front()
                    for (int i = 0; i < h->wnd.size(); ++i) {
                        if (h->wnd[i].first == ack->ID) {
                            h->wnd[i].second = h->FRtirgSeq.second+1;
                            break;
                        }
                    }
                    // activate the
                    activateLink(h->l, reP->me);
                }
            }
        }
        else if (h->phase == "FR") {
            if (!h->wnd.empty() && ack->ID > get<0>(h->wnd.front())) {
                ack->dp->timeOutOn = false;
                while (!h->wnd.empty() && ack->ID > get<0>(h->wnd.front())) {
                    h->wnd.erase(h->wnd.begin());
                }
                h->phase = "Normal";
                // adjust the wndSize
                if (h->timeOutHappenedInFR) {
                    h->cControl("TimeOut");
                    h->timeOutHappenedInFR = false;
                }
                else h->cControl("Exit FR without TO");
                // record
                WindSizeFiles[h->hostID] = fopen (WindSizeFileNames[h->f->flowID].c_str(), "a");
                fprintf(WindSizeFiles[h->hostID] , "%-10.2f %-10.2f\n", CUR_TIME, h->wndSize);
                fclose(WindSizeFiles[h->hostID] );
            }
            else {
                ack->dp->timeOutOn = false;
                h->cControl("Received in FR");
                WindSizeFiles[h->hostID] = fopen (WindSizeFileNames[h->f->flowID].c_str(), "a");
                fprintf(WindSizeFiles[h->hostID] , "%-10.2f %-10.2f\n", CUR_TIME, h->wndSize);
                fclose(WindSizeFiles[h->hostID] );
            }
            // if possible transmit new pkts
            int i = 1;
            while (h->wnd.size() < floor(h->wndSize) && h->f->remPktNum > 0 && i < 3) {
                // generate a new pkt by calling send(); not retransmit seqNo = 0, version = 1;
                Packet *newP = h->send(Data, CUR_TIME + scale1*i, 0, 1);
                // push the new generated pkt into vector pkts
                pkts.push_back(newP);
                // push into the wnd with version 1
                pair<int, int> pktPair(newP->ID, 1);
                h->wnd.push_back(pktPair);
                // a new PktTimeOut event should be set
                HostTimeOut *hto = new HostTimeOut(no, CUR_TIME+h->tOut + scale1*i, newP);
                PQ.push(hto);
                // activate the
                activateLink(h->l, newP->me);
                i++;
            }
        }
        //   delete p;
    }
    if (p->type == Data) {
        DataPkt *data = dynamic_cast<DataPkt *>(p);
        // receive pkt i and want pkt i+1
        if (data->ID == h->want && data->version >= h->wantVersion) {
            
            h->want++;
            while (!h->receiver.empty() && h->receiver.top() == h->want) {
                h->receiver.pop();
                h->want++;
            }
            if (h->receiver.empty()) {
                h->wantVersion = 1;
            }
        }
        else if (data->ID > h->want) {
            h->receiver.push(data->ID);
        }
        // send ACK the seqNo and version are 0, ID is want
        Packet *newP = h->send(ACK, CUR_TIME, 0, data->version, data);
        pkts.push_back(newP);
        // activate the
        activateLink(h->l, newP->me);
        
        PacketDelayFiles[h->f->flowID] = fopen (PacketDelayNames[h->f->flowID].c_str(), "a");
        fprintf(PacketDelayFiles[h->f->flowID] , "%-10.2f %-10.2f\n", CUR_TIME, data->tEnd - data->tStart);
        fclose(PacketDelayFiles[h->f->flowID] );
        // LOGGER
        FlowRateFiles[h->f->flowID] = fopen (FlowRateFileNames[h->f->flowID].c_str(), "a");
        fprintf(FlowRateFiles[h->f->flowID] , "%-10.2f %-10d %-10.2f\n", CUR_TIME, DATA_SIZE, hosts[0]->RTT);
        fclose(FlowRateFiles[h->f->flowID] );
    }
}
void HostFastUpdate::action() {
    int sum = 0;
    for (int i = 0; i < finishes.size(); ++i) {
        sum += finishes[i];
    }
    if (sum == 0)
        return;
    Host * h = hosts[no];
    h->wndSize = gama * (h->baseRTT/h->RTT * h->wndSize + alpha) + (1-gama)*h->wndSize;
    cout<<"hostFastUpdate wndSize "<<h->wndSize<<endl;
    HostFastUpdate *hfu = new HostFastUpdate(no, CUR_TIME + h->RTT + 1);
    PQ.push(hfu);
}

void LinkCostUpdate::action() {
    int sum = 0;
    for (int i = 0; i < finishes.size(); ++i) {
        sum += finishes[i];
    }
    if (sum == 0)
        return;
    Link *l = links[no];
    l->tempCost += (l->buffer1.size() + l->buffer2.size());
    LinkCostUpdate *lcu = new LinkCostUpdate(no, CUR_TIME + LCostUpdate_TIME);
    PQ.push(lcu);
}

void LinkAvailable::action() {
    Link *l = links[no];
    // one pkt finishes, we need to set tLeave for that pkt
    // compute the time and write into the file
    if (pktArrivesAtEnd) {
        arrP->tLeave = CUR_TIME;
        if (arrP->type == Data) {
            LinkRateFiles[no] = fopen (LinkRateFileNames[no].c_str(), "a");
            fprintf(LinkRateFiles[no], "%-10.2f %-10d\n", CUR_TIME, DATA_SIZE);
            fclose(LinkRateFiles[no]);
        }
        
        if (l->ns[direction].type == ROUTER) {
            //t+dT+1 is to avoid conflicts with la
            int routerNo = l->ns[direction].no;
            RouterReceive *rr = new RouterReceive(routerNo, CUR_TIME, arrP);
            PQ.push(rr);
        }
        else if (l->ns[direction].type == HOST && arrP->destNode.no == l->ns[direction].no && arrP->destNode.type == HOST) {
            int hostNo = l->ns[direction].no;
            HostReceive *hr = new HostReceive(hostNo, CUR_TIME, arrP);
            PQ.push(hr);
        }
    }
    else {
        l->workings[1-direction] = false;
    }
    Packet * p  = l->send(1-direction);
    if (p == NULL) return;
    /* create a new Event, t+dT links[no] will be available
     * i.e. the pkt sent by the above send() function arrives
     * CostInfo and DATA are DATA_SIZE
     * ACK is ACK_SIZE
     */
    BufferFiles[l->linkID] = fopen (BufferFileNames[l->linkID].c_str(), "a");
    fprintf(BufferFiles[l->linkID] , "%-10.2f %-10lu %-10lu\n", CUR_TIME, l->buffer1.size(), l->buffer2.size());
    fclose(BufferFiles[l->linkID] );
    float transmissionTime = (p->type == ACK) ? ACK_SIZE/l->rate : DATA_SIZE/l->rate;
    
    l->workings[1-direction] = true;
    LinkAvailable *la1 = new LinkAvailable(no, CUR_TIME + transmissionTime, direction, false, NULL);
    LinkAvailable *la2 = new LinkAvailable(no, CUR_TIME + l->delay + transmissionTime, direction, true, p);
    PQ.push(la1);
    PQ.push(la2);
}

/////////////////////// INITIALIZATIONS ////////////////////////
void init(const char *filename);
void routingTableInit();

void init(const char *filename) {
    ifstream is(filename);
    string line;
    vector<int> temp;
    
    //     parse flow;
    while (getline(is, line)) {
        if (line!="") {
            istringstream iss(line);
            string title;
            iss >> title;
            if (title == "flow") {
                int id = 0, srch = 0, desh = 0, srcr = 0, desr = 0, total_packets = 0, tstart = 0;
                iss >> id >> srch >> desh >> srcr >> desr >> total_packets >> tstart;
                int cur_id = 1;
                Flow* f = new Flow (id, srch, desh, srcr, desr, total_packets, cur_id, tstart);
                flows.push_back(f);
            }
        }else
            break;
    }
    
    for (int i = 0; i < flows.size(); i++) {
        finishes.push_back(1);
    }
    
    // parse nodes;
    while (getline(is, line)) {
        if (line!="") {
            istringstream iss(line);
            string title;
            iss >> title;
            if (title == "node") {
                int id = 0;
                string type;
                iss >> id >> type;
                
                nodeType notype = HOST;
                if (type == "ROUTER") {
                    notype = ROUTER;
                }
                Node *n = new Node(id, notype);
                nodes.push_back(n);
            }
        }else
            break;
    }
    
    // parse links;
    while (getline(is, line)) {
        if (line!="") {
            istringstream iss(line);
            string title;
            iss >> title;
            if (title == "link") {
                int id = 0, delay = 0, buffer_size = 0, n1_index = 0, n2_index = 0;
                float rate;
                iss >> id >> rate >> delay >> buffer_size >> n1_index >> n2_index;
                
                Node n1 = *nodes[n1_index];
                Node n2 = *nodes[n2_index];
                
                Link *l = new Link(id, rate, delay, buffer_size, n1, n2);
                links.push_back(l);
                
            }
        }else
            break;
    }
    
    // parse hosts;
    while (getline(is, line)) {
        if (line!="") {
            istringstream iss(line);
            string title;
            iss >> title;
            if (title == "host") {
                // is_left determine whether the host is on the left side
                // of the link:
                // 0 for left;
                // 1 for right;
                int id = 0, flow_id = 0, link_id = 0, is_left = 0;
                
                iss >> id >> flow_id >> link_id >> is_left;
                
                Host *h = new Host(id, flows[flow_id], links[link_id], is_left);
                hosts.push_back(h);
                
            }
        }else
            break;
    }
    
    // parse port;
    while (getline(is, line)) {
        if (line!="") {
            istringstream iss(line);
            string title;
            iss >> title;
            if (title == "port") {
                
                int id = 0, node_id = 0, link_id = 0, me = 0;
                iss >> id >> node_id >> link_id >> me;
                
                Port *p = new Port(*nodes[node_id], links[link_id], me);
                ports.push_back(p);
                
            }
        }else
            break;
    }
    
    vector<Port> router_ports;
    // parse router;
    while (getline(is, line)) {
        if (line!="") {
            temp.clear();
            router_ports.clear();
            istringstream iss(line);
            string title;
            iss >> title;
            if (title == "router") {
                int id = 0;
                iss >> id;
                int port_id = 0;
                while (iss >> port_id) {
                    router_ports.push_back(*ports[port_id]);
                }
                
                Router *r = new Router(id, router_ports);
                routers.push_back(r);
            }
        }else
            break;
    }
    
}
//=================================================
void routingTableInit() {
    // compute the initial routingTable
    for (int i = 0; i < routers.size(); ++i) {
        Router *r = routers[i];
        for (int j = 0; j < routers.size(); ++j) {
            Routing rt(INFINITY, INFINITY, INFINITY);
            r->routingTable.push_back(rt);
        }
        for (int j = 0; j < r->neighbors.size(); ++j) {
            int neighbor = r->neighbors[j];
            Node n(neighbor, ROUTER);
            int ptNo = r->getPortNo(n);
            Link *l = r->ports[ptNo].l;
            r->routingTable[neighbor].cost = l->delay * l->rate;
            r->routingTable[neighbor].nextHop = neighbor;
            for (int k = 0; k < r->ports.size(); ++k) {
                if (r->ports[k].n.no == neighbor) {
                    r->routingTable[neighbor].portNo = k;
                    break;
                }
            }
        }
        int I = r->routerID;
        r->routingTable[I].cost = 0;
        r->routingTable[I].nextHop = I;
        r->routingTable[I].portNo = 0;
        
        for (int j = 0; j < r->routingTable.size(); ++j) {
            r->costs.push_back(r->routingTable[j].cost);
            r->tempCosts.push_back(r->routingTable[j].cost);
        }
    }
    // send out the information
    for (int i = 0; i < routers.size(); ++i) {
        Router *r = routers[i];
        
        for (int j = 0; j < r->ports.size(); ++j) {
            if (r->ports[j].n.type == ROUTER) {
                // portNo is i and cp = NULL
                Packet * newPkt = r->send(CostInfo, CUR_TIME + i + scale1 * j, j, NULL);
                pkts.push_back(newPkt);
                RouterTimeOut *rto = new RouterTimeOut(r->routerID, CUR_TIME + TIME_OUT + i + scale1 * j, newPkt);
                PQ.push(rto);
                // activate the not-busy link
                Link *l = r->ports[j].l;
                activateLink(l, newPkt->me);
            }
        }
    }
    for (int i = 0; i < routers.size(); ++i) {
        changes.push_back(1);
    }
}

void deleteP() {
    for (int i = 0; i < TOpkts.size(); ++i) {
        delete pkts[i];
    }
}

void deleteF() {
    for (int i = 0; i < flows.size(); ++i) {
        delete flows[i];
    }
}

void deleteN() {
    for (int i = 0; i < nodes.size(); ++i) {
        delete nodes[i];
    }
}

void deleteL() {
    for (int i = 0; i < links.size(); ++i) {
        delete links[i];
    }
}

void deleteH() {
    for (int i = 0; i < hosts.size(); ++i) {
        delete hosts[i];
    }
}

void deletePort() {
    for (int i = 0; i < ports.size(); ++i) {
        delete ports[i];
    }
}

void deleteR() {
    for (int i = 0; i < routers.size(); ++i) {
        delete routers[i];
    }
}

///////////////////////////////////////////////////////////////////
//////////////////////////////// MAIN /////////////////////////////

int main(int argc, const char * argv[]) {
    //parse the file to initiate
    init(argv[1]);
    
    FILE LinkRate0, LinkRate1, LinkRate2, LinkRate3, LinkRate4, LinkRate5, LinkRate6, LinkRate7, LinkRate8;
    string lname0 = "LinkRate0.txt";
    string lname1 = "LinkRate1.txt";
    string lname2 = "LinkRate2.txt";
    string lname3 = "LinkRate3.txt";
    string lname4 = "LinkRate4.txt";
    string lname5 = "LinkRate5.txt";
    string lname6 = "LinkRate6.txt";
    string lname7 = "LinkRate7.txt";
    string lname8 = "LinkRate8.txt";
    
    FILE LinkBuffer0, LinkBuffer1, LinkBuffer2, LinkBuffer3, LinkBuffer4, LinkBuffer5, LinkBuffer6, LinkBuffer7, LinkBuffer8;
    string bname0 = "LinkBuffer0.txt";
    string bname1 = "LinkBuffer1.txt";
    string bname2 = "LinkBuffer2.txt";
    string bname3 = "LinkBuffer3.txt";
    string bname4 = "LinkBuffer4.txt";
    string bname5 = "LinkBuffer5.txt";
    string bname6 = "LinkBuffer6.txt";
    string bname7 = "LinkBuffer7.txt";
    string bname8 = "LinkBuffer8.txt";
    
    FILE FlowRate0, FlowRate1, FlowRate2;
    string fname0 = "FlowRate0.txt";
    string fname1 = "FlowRate1.txt";
    string fname2 = "FlowRate2.txt";
    
    FILE WindSize0, WindSize1, WindSize2;
    string wname0 = "WindSize0.txt";
    string wname1 = "WindSize1.txt";
    string wname2 = "WindSize2.txt";
    
    FILE PacketLoss0, PacketLoss1, PacketLoss2, PacketLoss3, PacketLoss4, PacketLoss5, PacketLoss6, PacketLoss7, PacketLoss8;
    string plname0 = "PacketLoss0.txt";
    string plname1 = "PacketLoss1.txt";
    string plname2 = "PacketLoss2.txt";
    string plname3 = "PacketLoss3.txt";
    string plname4 = "PacketLoss4.txt";
    string plname5 = "PacketLoss5.txt";
    string plname6 = "PacketLoss6.txt";
    string plname7 = "PacketLoss7.txt";
    string plname8 = "PacketLoss8.txt";
    
    FILE PacketDelay0, PacketDelay1, PacketDelay2;
    string pdname0 = "PacketDelay0.txt";
    string pdname1 = "PacketDelay1.txt";
    string pdname2 = "PacketDelay2.txt";
    
    LinkRateFiles.push_back(&LinkRate0);
    LinkRateFiles.push_back(&LinkRate1);
    LinkRateFiles.push_back(&LinkRate2);
    LinkRateFiles.push_back(&LinkRate3);
    LinkRateFiles.push_back(&LinkRate4);
    LinkRateFiles.push_back(&LinkRate5);
    LinkRateFiles.push_back(&LinkRate6);
    LinkRateFiles.push_back(&LinkRate7);
    LinkRateFiles.push_back(&LinkRate8);
    
    LinkRateFileNames.push_back(lname0);
    LinkRateFileNames.push_back(lname1);
    LinkRateFileNames.push_back(lname2);
    LinkRateFileNames.push_back(lname3);
    LinkRateFileNames.push_back(lname4);
    LinkRateFileNames.push_back(lname5);
    LinkRateFileNames.push_back(lname6);
    LinkRateFileNames.push_back(lname7);
    LinkRateFileNames.push_back(lname8);
    
    BufferFiles.push_back(&LinkBuffer0);
    BufferFiles.push_back(&LinkBuffer1);
    BufferFiles.push_back(&LinkBuffer2);
    BufferFiles.push_back(&LinkBuffer3);
    BufferFiles.push_back(&LinkBuffer4);
    BufferFiles.push_back(&LinkBuffer5);
    BufferFiles.push_back(&LinkBuffer6);
    BufferFiles.push_back(&LinkBuffer7);
    BufferFiles.push_back(&LinkBuffer8);
    
    BufferFileNames.push_back(bname0);
    BufferFileNames.push_back(bname1);
    BufferFileNames.push_back(bname2);
    BufferFileNames.push_back(bname3);
    BufferFileNames.push_back(bname4);
    BufferFileNames.push_back(bname5);
    BufferFileNames.push_back(bname6);
    BufferFileNames.push_back(bname7);
    BufferFileNames.push_back(bname8);
    
    PacketLossFiles.push_back(&PacketLoss0);
    PacketLossFiles.push_back(&PacketLoss1);
    PacketLossFiles.push_back(&PacketLoss2);
    PacketLossFiles.push_back(&PacketLoss3);
    PacketLossFiles.push_back(&PacketLoss4);
    PacketLossFiles.push_back(&PacketLoss5);
    PacketLossFiles.push_back(&PacketLoss6);
    PacketLossFiles.push_back(&PacketLoss7);
    PacketLossFiles.push_back(&PacketLoss8);
    
    PacketLossNames.push_back(plname0);
    PacketLossNames.push_back(plname1);
    PacketLossNames.push_back(plname2);
    PacketLossNames.push_back(plname3);
    PacketLossNames.push_back(plname4);
    PacketLossNames.push_back(plname5);
    PacketLossNames.push_back(plname6);
    PacketLossNames.push_back(plname7);
    PacketLossNames.push_back(plname8);
    
    PacketDelayFiles.push_back(&PacketDelay0);
    PacketDelayFiles.push_back(&PacketDelay1);
    PacketDelayFiles.push_back(&PacketDelay2);
    
    PacketDelayNames.push_back(pdname0);
    PacketDelayNames.push_back(pdname1);
    PacketDelayNames.push_back(pdname2);
    
    FlowRateFiles.push_back(&FlowRate0);
    FlowRateFiles.push_back(&FlowRate1);
    FlowRateFiles.push_back(&FlowRate2);
    
    FlowRateFileNames.push_back(fname0);
    FlowRateFileNames.push_back(fname1);
    FlowRateFileNames.push_back(fname2);
    
    WindSizeFiles.push_back(&WindSize0);
    WindSizeFiles.push_back(&WindSize1);
    WindSizeFiles.push_back(&WindSize2);
    
    WindSizeFileNames.push_back(wname0);
    WindSizeFileNames.push_back(wname1);
    WindSizeFileNames.push_back(wname2);
    routingTableInit();
    
    //while pQueue not empty, loop
    while (!PQ.empty()) {
        // fetch the top event
        Event * e = PQ.top();
        CUR_TIME = e->t;
        e->action();
        delete PQ.top();
        PQ.pop();
    }
    for (int i = 0; i < links.size(); ++i) {
        LinkCostUpdate *lcu = new LinkCostUpdate(i,CUR_TIME + i*scale1);
        PQ.push(lcu);
    }
    
    for (int i = 0; i < flows.size(); ++i) {
        HostReceive *rr = new HostReceive(flows[i]->srcHost, CUR_TIME + flows[i]->tStart, NULL);
        PQ.push(rr);
    }
    
    for (int i = 0; i < routers.size(); ++i) {
        RouterUpdate *ru = new RouterUpdate(i, CUR_TIME + UPDATE_TIME + i*scale1);
        PQ.push(ru);
        routers[i]->print();
    }
    
    while (!PQ.empty()) {
        // fetch the top event
        Event * e = PQ.top();
        CUR_TIME = e->t;
        e->action();
        delete PQ.top();
        PQ.pop();
    }
    
    deleteP();
    
    deleteF();
    deleteH();
    deleteL();
    deleteN();
    deletePort();
    deleteR();
    
}
