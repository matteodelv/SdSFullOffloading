/*
 * QueueSubclass.cc
 *
 *  Created on: Sep 27, 2019
 *      Author: matteo
 */

#include "QueueSubclass.h"
#include "Job.h"
//#include "OffloadedJob.h"

Define_Module(QueueSubclass);

bool QueueSubclass::endsBeforeNextStatusChange(simtime_t endTime) {
    return (endTime <= nextStatusChangeTime) ? true : false;
}

void QueueSubclass::updateNextStatusChangeTime() {
    simtime_t nextChange = par("changeStateDistribution").doubleValue();
    nextStatusChangeTime = simTime() + nextChange;
    scheduleAt(nextStatusChangeTime, wifiStatusMsg);
    EV << "Next WIFI status change time: " << nextStatusChangeTime << endl;
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
    updateNextStatusChangeTime();

    EV << "Called INITIALIZE on QueueSubclass\nInitial wifiAvailable = " << (wifiAvailable ? "ON" : "OFF") << "\n";
}

void QueueSubclass::handleMessage(cMessage *msg) {
    std::string jobName = msg->getName();
    if (jobName.std::string::compare("deadline_reached") == 0) {
        EV << "DEADLINE REACHED!" << endl;
        EV << "Message par list: " << msg->getParList() << endl;
    }
    else if (msg == wifiStatusMsg) {
        wifiAvailable = !wifiAvailable;
        EV << "WIFI STATUS CHANGED! Now is " << (wifiAvailable ? "ON" : "OFF") << "\n";

        // wifi OFF -> ON
        if (wifiAvailable) {
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

            updateNextStatusChangeTime();
        }
        // wifi ON -> OFF
        else {
            updateNextStatusChangeTime();
            if (servicedJob) {
                suspendService(servicedJob);
            }
            else {
                // Do nothing
            }
        }

    }
    else if (msg == endServiceMsg) {
        // TODO: Fix this call with servicedJob (vedi foglio)
        endService(servicedJob);

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
        //OffloadedJob *job = check_and_cast<OffloadedJob *>(msg);
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
    //getDisplayString().setTagArg("i", 1, queue.isEmpty() ? "" : "cyan");
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
        cMessage *deadline = new cMessage("deadline_reached");
        deadline->addObject(job->dup());
        simtime_t deadlineLength = par("deadlineDistribution").doubleValue();
        simtime_t deadlineTime = simTime() + deadlineLength;
        EV << "Deadline set for job " << job << "; firing time: " << deadlineTime << endl;
        scheduleAt(deadlineTime, deadline);
    }
}

simtime_t QueueSubclass::startService(Job *job) {
    simtime_t d = simTime() - job->getTimestamp();
    emit(queueingTimeSignal, d);
    job->setTotalQueueingTime(job->getTotalQueueingTime() + d);
    job->setTimestamp();
    EV << "Starting service of " << job->getName() << " - Current time: " << simTime() << endl;
    return par("serviceTime").doubleValue();
}

void QueueSubclass::endService(Job *job) {
    EV << "Finishing service of " << job->getName() << endl;
    simtime_t d = simTime() - job->getTimestamp();
    job->setTotalServiceTime(job->getTotalServiceTime() + d);
    send(job, "out");
}

void QueueSubclass::suspendService(Job *job) {
    // TODO: reschedule endServiceMsg to nextStatusChangeTime + job remaining processing time
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




