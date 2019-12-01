/*
 * LimitedSource.cc
 *
 *  Created on: Nov 24, 2019
 *      Author: matteo
 */

#include "LimitedSource.h"
#include "Job.h"


Define_Module(LimitedSource);

void LimitedSource::initialize()
{
    SourceBase::initialize();
    startTime = par("startTime");
    stopTime = par("stopTime");
    numJobs = -1;
    warmupExceeded = false;

    // schedule the first message timer for start time
    scheduleAt(startTime, new cMessage("newJobTimer"));
}

void LimitedSource::handleMessage(cMessage *msg)
{
    ASSERT(msg->isSelfMessage());

    if (simTime() >= getSimulation()->getWarmupPeriod() && !warmupExceeded) {
        numJobs = par("numJobs");
        jobCounter = 0;
        warmupExceeded = true;
    }

    if ((numJobs < 0 || numJobs > jobCounter) && (stopTime < 0 || stopTime > simTime())) {
        // reschedule the timer for the next message
        scheduleAt(simTime() + par("interArrivalTime").doubleValue(), msg);

        Job *job = createJob();
        send(job, "out");
    }
    else {
        // finished
        delete msg;
    }
}





