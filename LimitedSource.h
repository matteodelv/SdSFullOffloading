/*
 * LimitedSource.h
 *
 *  Created on: Nov 24, 2019
 *      Author: matteo
 */

#ifndef LIMITEDSOURCE_H_
#define LIMITEDSOURCE_H_

#include "QueueingDefs.h"
#include "Source.h"

using namespace queueing;


class QUEUEING_API LimitedSource : public SourceBase
{
    private:
        simtime_t startTime;
        simtime_t stopTime;
        int numJobs;
        bool warmupExceeded;
        bool transientAnalysis;

    protected:
        virtual void initialize() override;
        virtual void handleMessage(cMessage *msg) override;
};


#endif /* LIMITEDSOURCE_H_ */
