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
    warmupExceeded = false;
    transientAnalysis = par("transientAnalysis").boolValue();
    numJobs = transientAnalysis ? par("numJobs") : -1;

    // schedule the first message timer for start time
    scheduleAt(startTime, new cMessage("newJobTimer"));
}

void LimitedSource::handleMessage(cMessage *msg)
{
    ASSERT(msg->isSelfMessage());

    simtime_t warmup = getSimulation()->getWarmupPeriod();
    if (simTime() >= warmup && warmup > 0 && !warmupExceeded) {
        numJobs = par("numJobs");
        jobCounter = 0;
        warmupExceeded = true;
    }

    if ((numJobs < 0 || numJobs > jobCounter) && (stopTime < 0 || stopTime > simTime())) {
        // reschedule the timer for the next message
        scheduleAt(simTime() + par("interArrivalTime").doubleValue(), msg);

        Job *job = createJob();
        if (warmupExceeded || transientAnalysis)
            job->setKind(1);

        send(job, "out");
    }
    else {
        // finished
        delete msg;
    }
}





