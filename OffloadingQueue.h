/*
 * QueueSubclass.h
 *
 *  Created on: Sep 27, 2019
 *      Author: matteo
 */

#ifndef OFFLOADINGQUEUE_H_
#define OFFLOADINGQUEUE_H_

#include "QueueingDefs.h"
#include "Queue.h"
#include "Job.h"

using namespace queueing;


class QUEUEING_API OffloadingQueue : public cSimpleModule {
private:
    simsignal_t droppedSignal;
    simsignal_t queueLengthSignal;
    simsignal_t queueingTimeSignal;
    simsignal_t busySignal;

    simsignal_t wifiActiveTime;
    simsignal_t cellActiveTime;
    simsignal_t deadlineDistrib;
    simsignal_t jobServiceTimeSignal;

    Job *servicedJob;
    cMessage *endServiceMsg;
    cQueue queue;
    int capacity;
    bool fifo;

    bool wifiAvailable;
    cMessage *wifiStatusMsg;
    Job *suspendedJob;

    Job *getFromQueue();

    simtime_t nextStatusChangeTime;
    simtime_t curJobServiceTime = SIMTIME_ZERO;

    void updateNextStatusChangeTime(simtime_t expWiFiEnd);

public:
    OffloadingQueue();
    virtual ~OffloadingQueue();
    int length();
    static int compareFunction(cObject *a, cObject *b);

protected:
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
    virtual void refreshDisplay() const override;
    virtual void finish() override;

    virtual void arrival(Job *job);
    virtual simtime_t startService(Job *job);
    virtual void suspendService(Job *job);
    virtual void resumeService(Job *job);
    virtual void endService(Job *job, int outGate);
};



#endif /* OFFLOADINGQUEUE_H_ */
