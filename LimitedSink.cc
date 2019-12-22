/*
 * LimitedLimitedSink.cc
 *
 *  Created on: Dec 16, 2019
 *      Author: matteo
 */

#include "LimitedSink.h"
#include "Job.h"


Define_Module(LimitedSink);

void LimitedSink::initialize()
{
    lifeTimeSignal = registerSignal("lifeTime");
    totalQueueingTimeSignal = registerSignal("totalQueueingTime");
    queuesVisitedSignal = registerSignal("queuesVisited");
    totalServiceTimeSignal = registerSignal("totalServiceTime");
    totalDelayTimeSignal = registerSignal("totalDelayTime");
    delaysVisitedSignal = registerSignal("delaysVisited");
    generationSignal = registerSignal("generation");
    keepJobs = par("keepJobs");

    totalResponseTime = registerSignal("totalResponseTime");

    jobCounter = 0;
    WATCH(jobCounter);
}

void LimitedSink::handleMessage(cMessage *msg)
{
    Job *job = check_and_cast<Job *>(msg);

    if (job->getKind() == 1)
        jobCounter++;

    // gather statistics
    if (job->getKind() == 1) {
        emit(totalResponseTime, job->getTotalQueueingTime() + job->getTotalServiceTime());

        emit(lifeTimeSignal, simTime()- job->getCreationTime());
        emit(totalQueueingTimeSignal, job->getTotalQueueingTime());
        emit(queuesVisitedSignal, job->getQueueCount());
        emit(totalServiceTimeSignal, job->getTotalServiceTime());
        emit(totalDelayTimeSignal, job->getTotalDelayTime());
        emit(delaysVisitedSignal, job->getDelayCount());
        emit(generationSignal, job->getGeneration());
    }

    if (!keepJobs)
        delete msg;

    if (jobCounter >= par("numJobs").intValue() && getSimulation()->getWarmupPeriod() != 0.0)
        endSimulation();
}

void LimitedSink::finish()
{
    // TODO missing scalar statistics
}


