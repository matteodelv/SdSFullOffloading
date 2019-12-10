/*
 * QueueSubclass.cc
 *
 *  Created on: Nov 18, 2019
 *      Author: matteo
 */

#include "QueueCustom.h"
#include "Job.h"

Define_Module(QueueCustom);

QueueCustom::QueueCustom()
{
    jobServiced = nullptr;
    endServiceMsg = nullptr;
}

QueueCustom::~QueueCustom()
{
    delete jobServiced;
    cancelAndDelete(endServiceMsg);
}

void QueueCustom::initialize()
{
    droppedSignal = registerSignal("dropped");
    queueingTimeSignal = registerSignal("queueingTime");
    queueLengthSignal = registerSignal("queueLength");
    emit(queueLengthSignal, 0);
    busySignal = registerSignal("busy");
    emit(busySignal, false);

    endServiceMsg = new cMessage("end-service");
    fifo = par("fifo");
    capacity = par("capacity");
    queue.setName("queue");

    remoteTracking = par("remoteTracking");
    responseTimeSignal = registerSignal("responseTime");
}

void QueueCustom::handleMessage(cMessage *msg)
{
    if (msg == endServiceMsg) {
        endService(jobServiced);
        if (queue.isEmpty()) {
            jobServiced = nullptr;
            emit(busySignal, false);
        }
        else {
            jobServiced = getFromQueue();
            emit(queueLengthSignal, length());
            simtime_t serviceTime = startService(jobServiced);
            scheduleAt(simTime()+serviceTime, endServiceMsg);
        }
    }
    else {
        Job *job = check_and_cast<Job *>(msg);
        arrival(job);

        if (!jobServiced) {
            // processor was idle
            jobServiced = job;
            emit(busySignal, true);
            simtime_t serviceTime = startService(jobServiced);
            scheduleAt(simTime()+serviceTime, endServiceMsg);
        }
        else {
            // check for container capacity
            if (capacity >= 0 && queue.getLength() >= capacity) {
                //EV << "Capacity full! Job dropped.\n";
                if (hasGUI())
                    bubble("Dropped!");
                emit(droppedSignal, 1);
                delete job;
                return;
            }
            queue.insert(job);
            emit(queueLengthSignal, length());
            // Removed queue count update to avoid duplicate data
        }
    }
}

void QueueCustom::refreshDisplay() const
{
    getDisplayString().setTagArg("i2", 0, jobServiced ? "status/execute" : "");
    getDisplayString().setTagArg("i", 1, queue.isEmpty() ? "" : "cyan");
}

Job *QueueCustom::getFromQueue()
{
    Job *job;
    if (fifo) {
        job = (Job *)queue.pop();
    }
    else {
        job = (Job *)queue.back();
        // FIXME this may have bad performance as remove uses linear search
        queue.remove(job);
    }
    return job;
}

int QueueCustom::length()
{
    return queue.getLength();
}

void QueueCustom::arrival(Job *job)
{
    job->setTimestamp();
    job->setQueueCount(job->getQueueCount() + 1);
    EV << job << " queue count: " << job->getQueueCount() << endl;
}

simtime_t QueueCustom::startService(Job *job)
{
    // gather queueing time statistics
    simtime_t d = simTime() - job->getTimestamp();
    emit(queueingTimeSignal, d);

    if (!remoteTracking) {
        job->setTotalQueueingTime(job->getTotalQueueingTime() + d);
        job->setTimestamp();
        EV << job << " - total queueing time: " << job->getTotalQueueingTime() << endl;
    }

    EV << "Starting service of " << job->getName() << endl;

    return par("serviceTime").doubleValue();
}

void QueueCustom::endService(Job *job)
{
    EV << "Finishing service of " << job->getName() << endl;

    simtime_t d = simTime() - job->getTimestamp();

    if (remoteTracking)
        emit(responseTimeSignal, d);
    else {
        job->setTotalServiceTime(job->getTotalServiceTime() + d);
        simtime_t cellRespTime = job->getTotalQueueingTime() + job->getTotalServiceTime();
        emit(responseTimeSignal, cellRespTime);
    }

    send(job, "out");
}

void QueueCustom::finish()
{
}

