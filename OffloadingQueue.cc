/*
 * QueueSubclass.cc
 *
 *  Created on: Sep 27, 2019
 *      Author: matteo
 */

#include "OffloadingQueue.h"
#include <assert.h>

#include "Job.h"

Define_Module(OffloadingQueue);

int OffloadingQueue::compareFunction(cObject *a, cObject *b) {
    Job *jobA = check_and_cast<Job *>(a);
    Job *jobB = check_and_cast<Job *>(b);
    cMessage *aDeadline = (cMessage *)jobA->getContextPointer();
    cMessage *bDeadline = (cMessage *)jobB->getContextPointer();
    if (aDeadline && bDeadline) {
        simtime_t now = simTime();
        simtime_t timeLeftA = aDeadline->getArrivalTime() - now;
        simtime_t timeLeftB = bDeadline->getArrivalTime() - now;
        assert (timeLeftA > SIMTIME_ZERO);
        assert (timeLeftB > SIMTIME_ZERO);
        if (timeLeftA < timeLeftB) return -1;
        else if (timeLeftA > timeLeftB) return 1;
        else return 0;
    }
    else if (aDeadline || bDeadline) {
        if (aDeadline) return -1;
        else return 1;
    }
    else {
        simtime_t creationTimeA = jobA->getCreationTime();
        simtime_t creationTimeB = jobB->getCreationTime();
        if (creationTimeA < creationTimeB) return -1;
        else if (creationTimeA > creationTimeB) return 1;
        else return 0;
    }
}

void OffloadingQueue::updateNextStatusChangeTime(simtime_t expWiFiEnd) {
    if (wifiAvailable && expWiFiEnd != SIMTIME_ZERO)
        emit(wifiActiveTime, expWiFiEnd);

    simtime_t nextChange = (wifiAvailable) ? expWiFiEnd : par("cellularStateDistribution").doubleValue();
    if (!wifiAvailable)
        emit(cellActiveTime, nextChange);
    nextStatusChangeTime = simTime() + nextChange;
    scheduleAt(nextStatusChangeTime, wifiStatusMsg);
    EV << "Next WIFI status change time: " << nextStatusChangeTime << endl;
}

OffloadingQueue::OffloadingQueue() {
    servicedJob = nullptr;
    endServiceMsg = nullptr;
    wifiStatusMsg = nullptr;
    suspendedJob = nullptr;
}

OffloadingQueue::~OffloadingQueue() {
    delete servicedJob;
    delete suspendedJob;
    cancelAndDelete(endServiceMsg);
    cancelAndDelete(wifiStatusMsg);
}

void OffloadingQueue::initialize() {
    droppedSignal = registerSignal("dropped");
    queueingTimeSignal = registerSignal("queueingTime");
    queueLengthSignal = registerSignal("queueLength");
    emit(queueLengthSignal, 0);

    busySignal = registerSignal("busy");
    emit(busySignal, false);

    wifiActiveTime = registerSignal("wifiActiveTime");
    cellActiveTime = registerSignal("cellActiveTime");
    deadlineDistrib = registerSignal("deadlineDistrib");
    jobServiceTimeSignal = registerSignal("jobServiceTime");

    endServiceMsg = new cMessage("end_service");
    fifo = par("fifo");
    capacity = par("capacity");
    queue.setName("queue");
    queue.setup(compareFunction);

    wifiAvailable = false;
    wifiStatusMsg = new cMessage("wifi_status_changed");
    updateNextStatusChangeTime(SIMTIME_ZERO);

    EV << "Called INITIALIZE on QueueSubclass\nInitial wifiAvailable = " << (wifiAvailable ? "ON" : "OFF") << "\n";
}

void OffloadingQueue::handleMessage(cMessage *msg) {
    std::string jobName = msg->getName();
    if (jobName.std::string::compare("deadline_reached") == 0) {
        if (msg->getContextPointer()) {
            Job *job = (Job *)msg->getContextPointer();
            msg->setContextPointer(nullptr);
            EV << "DEADLINE REACHED! WIFI status: " << wifiAvailable << " - Associated job: " << job << endl;

            if (hasGUI()) {
                std::string text = std::string("Deadline for ") + std::string(job->getName());
                bubble(text.c_str());
            }

            if (job == servicedJob) {
                if (endServiceMsg->isScheduled())
                    cancelEvent(endServiceMsg);

                if (queue.isEmpty()) {
                    servicedJob = nullptr;
                    emit(busySignal, false);
                }
                else {
                    servicedJob = getFromQueue();
                    emit(queueLengthSignal, length());
                    simtime_t serviceTime = startService(servicedJob);
                    simtime_t nextSchedule = simTime() + serviceTime;
                    curJobServiceTime = serviceTime;
                    scheduleAt(nextSchedule, endServiceMsg);
                    EV << "Job service time: " << curJobServiceTime << endl;
                    EV << "END time for " << servicedJob << ": " << nextSchedule << endl;
                }
            }
            else if (job == suspendedJob) {
                suspendedJob = nullptr;
                if (endServiceMsg->isScheduled())
                    cancelEvent(endServiceMsg);
            }
            else
                queue.remove(job);

            emit(queueLengthSignal, length());
            send(job, "out", 0);
            delete msg;
        }
        else
            EV << "DEADLINE REACHED!" << endl;
    }
    else if (msg == wifiStatusMsg) {
        wifiAvailable = !wifiAvailable;
        EV << "WIFI STATUS CHANGED! Now is " << (wifiAvailable ? "ON" : "OFF") << "\n";

        // wifi OFF -> ON
        if (wifiAvailable) {
            simtime_t nextWiFiEnd = par("wifiStateDistribution").doubleValue();

            if (suspendedJob)
                resumeService(suspendedJob);
            // no jobs were suspended so get a new one, if available
            else {
                if (queue.isEmpty()) {
                    servicedJob = nullptr;
                    emit(busySignal, false);
                }
                else {
                    servicedJob = getFromQueue();
                    emit(queueLengthSignal, length());
                    simtime_t serviceTime = startService(servicedJob);
                    simtime_t nextSchedule = simTime() + serviceTime;
                    curJobServiceTime = serviceTime;
                    scheduleAt(nextSchedule, endServiceMsg);
                    EV << "Job service time: " << curJobServiceTime << endl;
                    EV << "END time for " << servicedJob << ": " << nextSchedule << endl;
                }
            }

            updateNextStatusChangeTime(nextWiFiEnd);
        }
        // wifi ON -> OFF
        else {
            updateNextStatusChangeTime(SIMTIME_ZERO);

            // update queueing time here
            cQueue::Iterator it = cQueue::Iterator(queue);
            while (!it.end()) {
                Job *job = check_and_cast<Job *>(*it);
                simtime_t delta = simTime() - job->getTimestamp();
                job->setTotalQueueingTime(job->getTotalQueueingTime() + delta);
                ++it;
            }

            if (servicedJob)
                suspendService(servicedJob);
        }
    }
    else if (msg == endServiceMsg) {
        endService(servicedJob, 1);

        if (queue.isEmpty()) {
            servicedJob = nullptr;
            emit(busySignal, false);
        }
        else {
            servicedJob = getFromQueue();
            emit(queueLengthSignal, length());
            simtime_t serviceTime = startService(servicedJob);
            simtime_t nextSchedule = simTime() + serviceTime;
            curJobServiceTime = serviceTime;
            scheduleAt(nextSchedule, endServiceMsg);
            EV << "Job service time: " << curJobServiceTime << endl;
            EV << "END time for " << servicedJob << ": " << nextSchedule << endl;
        }
    }
    else {
        Job *job = check_and_cast<Job *>(msg);
        arrival(job);

        if (wifiAvailable) {
            if (!servicedJob) {
                servicedJob = job;
                emit(busySignal, true);
                simtime_t serviceTime = startService(servicedJob);
                simtime_t nextSchedule = simTime() + serviceTime;
                curJobServiceTime = serviceTime;
                scheduleAt(nextSchedule, endServiceMsg);
                EV << "Job service time: " << curJobServiceTime << endl;
                EV << "END time for " << servicedJob << ": " << nextSchedule << endl;
            }
            else {
                if (capacity >= 0 && queue.getLength() >= capacity) {
                    EV << "Capacity full! Job dropped...\n";
                    if (hasGUI())
                        bubble("Dropped!");

                    emit(droppedSignal, 1);
                    delete job;
                    return;
                }

                queue.insert(job);
                emit(queueLengthSignal, length());
            }
        }
        else {
            queue.insert(job);
            emit(queueLengthSignal, length());
        }
    }
}

void OffloadingQueue::refreshDisplay() const {
    getDisplayString().setTagArg("i2", 0, servicedJob ? "status/execute" : "");
    getDisplayString().setTagArg("i", 1, wifiAvailable ? "lime" : "red");
}

Job* OffloadingQueue::getFromQueue() {
    Job *job;
    if (fifo)
        job = (Job *)queue.pop();
    else {
        job = (Job *)queue.back();
        queue.remove(job);
    }
    EV << "Getting job from queue: " << job << endl;
    return job;
}

int OffloadingQueue::length() {
    return queue.getLength();
}

void OffloadingQueue::arrival(Job *job) {
    job->setTimestamp();
    job->setQueueCount(job->getQueueCount() + 1);
    EV << job << " queue count: " << job->getQueueCount() << endl;

    // WIFI is OFF so add deadline to jobs
    if (!wifiAvailable) {
        cMessage *deadlineMsg = new cMessage("deadline_reached");
        deadlineMsg->setContextPointer(job);
        job->setContextPointer(deadlineMsg);

        simtime_t deadlineLength = par("deadlineDistribution").doubleValue();
        emit(deadlineDistrib, deadlineLength);
        simtime_t deadlineTime = simTime() + deadlineLength;
        EV << "Deadline set for job " << job << "; firing time: " << deadlineTime << endl;
        scheduleAt(deadlineTime, deadlineMsg);
    }
}

simtime_t OffloadingQueue::startService(Job *job) {
    EV << "Starting service of " << job->getName() << " - Current time: " << simTime() << endl;

    simtime_t delta = simTime() - job->getTimestamp();
    job->setTotalQueueingTime(job->getTotalQueueingTime() + delta);
    job->setTimestamp();

    return par("serviceTime").doubleValue();
}

void OffloadingQueue::endService(Job *job, int gateID) {
    EV << "Finishing service of " << job->getName() << endl;

    if (job->getContextPointer()) {
        cMessage *deadlineMsg = (cMessage *)job->getContextPointer();
        if (deadlineMsg->isScheduled())
            cancelAndDelete(deadlineMsg);
        job->setContextPointer(nullptr);
    }

    simtime_t delta = simTime() - job->getTimestamp();
    job->setTotalServiceTime(job->getTotalServiceTime() + delta);

    EV << job << " - queueing time: " << job->getTotalQueueingTime() << " - service time: " << job->getTotalServiceTime() << endl;

    if (job->getKind() == 1)
        emit(jobServiceTimeSignal, job->getTotalServiceTime());

    send(job, "out", gateID);
}

void OffloadingQueue::suspendService(Job *job) {
    cancelEvent(endServiceMsg);
    simtime_t startTime = job->getTimestamp();
    simtime_t elapsedTime = simTime() - startTime;
    simtime_t remainingTime = curJobServiceTime - elapsedTime;
    scheduleAt(nextStatusChangeTime + remainingTime, endServiceMsg);
    job->setTotalServiceTime(job->getTotalServiceTime() + elapsedTime);

    job->setTimestamp();
    suspendedJob = job;
    servicedJob = nullptr;
    EV << "Service SUSPENDED! Received: " << job << " - Suspended: " << suspendedJob << " - Serviced: " << servicedJob << endl;
    EV << "Elapsed service time for " << job << ": " << elapsedTime << endl;
    EV << "Remaining service time for " << job << ": " << remainingTime << endl;
    EV << "New scheduleAt time: " << nextStatusChangeTime + remainingTime << endl;
}

void OffloadingQueue::resumeService(Job *job) {
    simtime_t suspendedTime = simTime() - job->getTimestamp();
    job->setTotalQueueingTime(job->getTotalQueueingTime() + suspendedTime);
    job->setTimestamp();
    servicedJob = job;
    suspendedJob = nullptr;
    EV << "Service RESUMED! Received: " << job << " - Suspended: " << suspendedJob << " - Serviced: " << servicedJob << endl;
    EV << "Current time: " << simTime() << endl;
}

void OffloadingQueue::finish() {

}




