/*
 * QueueSubclass.h
 *
 *  Created on: Nov 18, 2019
 *      Author: matteo
 */

#ifndef QUEUECUSTOM_H_
#define QUEUECUSTOM_H_

#include "QueueingDefs.h"
#include "Queue.h"
#include "Job.h"

using namespace queueing;


/**
 * Abstract base class for single-server queues.
 */
class QUEUEING_API QueueCustom : public cSimpleModule
{
    private:
        simsignal_t droppedSignal;
        simsignal_t queueLengthSignal;
        simsignal_t queueingTimeSignal;
        simsignal_t busySignal;

        simsignal_t responseTimeSignal;

        Job *jobServiced;
        cMessage *endServiceMsg;
        cQueue queue;
        int capacity;
        bool fifo;
        bool remoteTracking;

        Job *getFromQueue();

    public:
        QueueCustom();
        virtual ~QueueCustom();
        int length();

    protected:
        virtual void initialize() override;
        virtual void handleMessage(cMessage *msg) override;
        virtual void refreshDisplay() const override;
        virtual void finish() override;

        // hook functions to (re)define behaviour
        virtual void arrival(Job *job);
        virtual simtime_t startService(Job *job);
        virtual void endService(Job *job);
};


#endif /* QUEUECUSTOM_H_ */
