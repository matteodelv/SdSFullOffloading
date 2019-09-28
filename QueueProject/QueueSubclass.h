/*
 * QueueSubclass.h
 *
 *  Created on: Sep 27, 2019
 *      Author: matteo
 */

#ifndef QUEUESUBCLASS_H_
#define QUEUESUBCLASS_H_

#include "QueueingDefs.h"
#import "Queue.h"
#import "Job.h"

using namespace queueing;

//class Job;


class QUEUEING_API QueueSubclass : public cSimpleModule {
private:
    simsignal_t droppedSignal;
    simsignal_t queueLengthSignal;
    simsignal_t queueingTimeSignal;
    simsignal_t busySignal;

    Job *jobServiced;
    cMessage *endServiceMsg;
    cQueue queue;
    int capacity;
    bool fifo;

    bool wifiAvailable;
    cMessage *wifiStatusMsg;
    Job *suspendedJob;

    Job *getFromQueue();

public:
    QueueSubclass();
    virtual ~QueueSubclass();
    int length();

protected:
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
    virtual void refreshDisplay() const override;
    virtual void finish() override;

    virtual void arrival(Job *job);
    virtual simtime_t startService(Job *job);
    virtual void suspendService(Job *job); // va bene void oppure è meglio ritornare simtime_t?
    virtual void endService(Job *job);
};

//#import <omnetpp.h>
//#import "Job.h"
//#import "Queue.h"
//
//using namespace queueing;
//
//class QueueSubclass : public Queue {
//
//public:
//    QueueSubclass();
//    virtual ~QueueSubclass();
///*
//protected:
//    virtual void initialize() override;
//    virtual void handleMessage(cMessage *msg) override;
//    virtual void refreshDisplay() const override;
//    virtual void finish() override;
//
//    // hook functions to (re)define behaviour
//    virtual void arrival(Job *job);
//    virtual simtime_t startService(Job *job);
//    virtual void endService(Job *job);
//*/
//
//protected:
//    virtual void initialize() override;
//    virtual void handleMessage(cMessage *msg) override;
//
//    virtual void arrival(Job *job) override;
//    virtual simtime_t startService(Job *job) override;
//    virtual void endService(Job *job) override;
//
//    bool wifiAvailable;
//    cMessage *wifiStatusMsg;
//    Job *jobServiced;
//};



#endif /* QUEUESUBCLASS_H_ */
