/*
 * QueueSubclass.cc
 *
 *  Created on: Sep 27, 2019
 *      Author: matteo
 */

#include "QueueSubclass.h"
#include "Job.h"

typedef enum {
    PolicyTypeDelete = 1,
    PolicyTypeShift
} PolicyType;

Define_Module(QueueSubclass);

void QueueSubclass::updateNextStatusChangeTime(simtime_t expWiFiEnd) {
    simtime_t nextChange = (wifiAvailable) ? expWiFiEnd : par("cellularStateDistribution").doubleValue();
    nextStatusChangeTime = simTime() + nextChange;
    scheduleAt(nextStatusChangeTime, wifiStatusMsg);
    EV << "Next WIFI status change time: " << nextStatusChangeTime << endl;
    //EV << "Delta: " << nextChange << " - from " << ((wifiAvailable) ? "WIFI" : "CELLULAR") << endl;
}

QueueSubclass::QueueSubclass() {
    servicedJob = nullptr;
    endServiceMsg = nullptr;
    wifiStatusMsg = nullptr;
    suspendedJob = nullptr;
}

QueueSubclass::~QueueSubclass() {
    delete servicedJob;
    delete suspendedJob;
    cancelAndDelete(endServiceMsg);
    cancelAndDelete(wifiStatusMsg);
}

void QueueSubclass::initialize() {
    droppedSignal = registerSignal("dropped");
    queueingTimeSignal = registerSignal("queueingTime");
    queueLengthSignal = registerSignal("queueLength");
    emit(queueLengthSignal, 0);

    busySignal = registerSignal("busy");
    emit(busySignal, false);

    endServiceMsg = new cMessage("end_service");
    fifo = par("fifo");
    capacity = par("capacity");
    queue.setName("queue");

    wifiAvailable = false;
    wifiStatusMsg = new cMessage("wifi_status_changed");
    updateNextStatusChangeTime(SIMTIME_ZERO);

    EV << "Called INITIALIZE on QueueSubclass\nInitial wifiAvailable = " << (wifiAvailable ? "ON" : "OFF") << "\n";
}

void QueueSubclass::handleMessage(cMessage *msg) {
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

            if (job == servicedJob && par("policyType").intValue() == PolicyTypeShift) {
                endService(servicedJob, 0);
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
            else {
                // This happens only when wifi is OFF so the check if not required (if policyType == 1)
                queue.remove(job);
                job->setTotalServiceTime(0);
                emit(queueLengthSignal, length());
                send(job, "out", 0);
                delete msg;
            }

//            if (hasGUI()) {
//                std::string text = std::string(job->getName()) + std::string(" dropped");
//                bubble(text.c_str());
//            }


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

            if (par("policyType").intValue() == PolicyTypeDelete) {
                // Cancel all deadlines for queued jobs
                cQueue::Iterator it = cQueue::Iterator(queue);
                while (!it.end()) {
                    Job *job = check_and_cast<Job *>(*it);
                    if (job->getContextPointer()) {
                        cMessage *deadlineMsg = (cMessage *)job->getContextPointer();
                        if (deadlineMsg->isScheduled())
                            cancelAndDelete(deadlineMsg);
                        job->setContextPointer(nullptr);
                        EV << "Deleted scheduled deadline for " << job << endl;
                    }
                    ++it;
                }
            }
            else {
                // Shift all deadlines for queued jobs
                cQueue::Iterator it = cQueue::Iterator(queue);
                while(!it.end()) {
                    Job *job = check_and_cast<Job *>(*it);
                    if (job->getContextPointer()) {
                        cMessage *deadlineMsg = (cMessage *)job->getContextPointer();
                        simtime_t prevSchedTime = deadlineMsg->getArrivalTime();
                        if (deadlineMsg->isScheduled())
                            cancelEvent(deadlineMsg);
                        //scheduleAt(prevSchedTime + wifiDeltaTime, deadlineMsg);
                        simtime_t shiftedTime = prevSchedTime + nextWiFiEnd;
                        scheduleAt(shiftedTime, deadlineMsg);
                        job->setContextPointer(deadlineMsg);
                        EV << "Shifted deadline for " << job << " - Old time: " << prevSchedTime << " - New time: " << deadlineMsg->getArrivalTime() << endl;
                    }
                    ++it;
                }
            }

            if (suspendedJob) {
                resumeService(suspendedJob);
            }
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
            if (servicedJob) {
                suspendService(servicedJob);
            }
            else {
                // Do nothing
            }
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
                job->setQueueCount(job->getQueueCount() + 1);
            }
        }
        else {
            queue.insert(job);
            emit(queueLengthSignal, length());
            job->setQueueCount(job->getQueueCount() + 1);
        }
    }
}

void QueueSubclass::refreshDisplay() const {
    getDisplayString().setTagArg("i2", 0, servicedJob ? "status/execute" : "");
    getDisplayString().setTagArg("i", 1, wifiAvailable ? (queue.isEmpty() ? "" : "cyan") : "red");
}

Job* QueueSubclass::getFromQueue() {
    Job *job;
    if (fifo)
        job = (Job *)queue.pop();
    else {
        job = (Job *)queue.back();
        queue.remove(job);
    }
    return job;
}

int QueueSubclass::length() {
    return queue.getLength();
}

void QueueSubclass::arrival(Job *job) {
    job->setTimestamp();

    // WIFI is OFF so add deadline to jobs
    if (!wifiAvailable) {
        cMessage *deadlineMsg = new cMessage("deadline_reached");
        deadlineMsg->setContextPointer(job);
        job->setContextPointer(deadlineMsg);

        simtime_t deadlineLength = par("deadlineDistribution").doubleValue();
        simtime_t deadlineTime = simTime() + deadlineLength;
        EV << "Deadline set for job " << job << "; firing time: " << deadlineTime << endl;
        scheduleAt(deadlineTime, deadlineMsg);
    }
}

simtime_t QueueSubclass::startService(Job *job) {
    simtime_t d = simTime() - job->getTimestamp();
    emit(queueingTimeSignal, d);
    job->setTotalQueueingTime(job->getTotalQueueingTime() + d);
    job->setTimestamp();
    EV << "Starting service of " << job->getName() << " - Current time: " << simTime() << endl;

    // Assumption: when a job is getting served, it can't leave the queue anymore
    if (job->getContextPointer()) {
        cMessage *deadlineMsg = (cMessage *)job->getContextPointer();
        if (deadlineMsg->isScheduled())
            cancelAndDelete(deadlineMsg);
        job->setContextPointer(nullptr);
    }

    return par("serviceTime").doubleValue();
}

void QueueSubclass::endService(Job *job, int gateID) {
    EV << "Finishing service of " << job->getName() << endl;

    simtime_t d = simTime() - job->getTimestamp();
    job->setTotalServiceTime(job->getTotalServiceTime() + d);
    send(job, "out", gateID);
}

void QueueSubclass::suspendService(Job *job) {
    // TODO: non considerare i tempi di sospensione come serviceTime
    cancelEvent(endServiceMsg);
    simtime_t startTime = job->getTimestamp();
    simtime_t elapsedTime = simTime() - startTime;
    simtime_t remainingTime = curJobServiceTime - elapsedTime;
    scheduleAt(nextStatusChangeTime + remainingTime, endServiceMsg);

    job->setTimestamp();
    suspendedJob = job;
    servicedJob = nullptr;
    EV << "Service SUSPENDED! Received: " << job << " - Suspended: " << suspendedJob << " - Serviced: " << servicedJob << endl;
    EV << "Elapsed service time for " << job << ": " << elapsedTime << endl;
    EV << "Remaining service time for " << job << ": " << remainingTime << endl;
    EV << "New scheduleAt time: " << nextStatusChangeTime + remainingTime << endl;
}

void QueueSubclass::resumeService(Job *job) {
    job->setTimestamp();
    servicedJob = job;
    suspendedJob = nullptr;
    EV << "Service RESUMED! Received: " << job << " - Suspended: " << suspendedJob << " - Serviced: " << servicedJob << endl;
    EV << "Current time: " << simTime() << endl;
}

void QueueSubclass::finish() {

}




