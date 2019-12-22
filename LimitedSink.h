/*
 * LimitedSink.h
 *
 *  Created on: Dec 16, 2019
 *      Author: matteo
 */

#ifndef LIMITEDSINK_H_
#define LIMITEDSINK_H_

#include "QueueingDefs.h"
#include "Job.h"

using namespace queueing;

/**
 * Consumes jobs; see NED file for more info.
 */
class QUEUEING_API LimitedSink : public cSimpleModule
{
  private:
    simsignal_t lifeTimeSignal;
    simsignal_t totalQueueingTimeSignal;
    simsignal_t queuesVisitedSignal;
    simsignal_t totalServiceTimeSignal;
    simsignal_t totalDelayTimeSignal;
    simsignal_t delaysVisitedSignal;
    simsignal_t generationSignal;
    bool keepJobs;
    int jobCounter;

    simsignal_t totalResponseTime;

  protected:
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
    virtual void finish() override;
};



#endif /* LIMITEDSINK_H_ */
