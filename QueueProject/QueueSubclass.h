/*
 * QueueSubclass.h
 *
 *  Created on: Sep 27, 2019
 *      Author: matteo
 */

#ifndef QUEUESUBCLASS_H_
#define QUEUESUBCLASS_H_

#include "QueueingDefs.h"
#include "Queue.h"
#include "Job.h"

using namespace queueing;

//class Job;


class QUEUEING_API QueueSubclass : public cSimpleModule {
private:
    simsignal_t droppedSignal;
    simsignal_t queueLengthSignal;
    simsignal_t queueingTimeSignal;
    simsignal_t busySignal;

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

    bool endsBeforeNextStatusChange(simtime_t endTime);
    void updateNextStatusChangeTime();

    Job *clonedJob;

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
    virtual void resumeService(Job *job);
    virtual void endService(Job *job);
};



#endif /* QUEUESUBCLASS_H_ */
