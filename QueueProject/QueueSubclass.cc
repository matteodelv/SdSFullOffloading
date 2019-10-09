/*
 * QueueSubclass.cc
 *
 *  Created on: Sep 27, 2019
 *      Author: matteo
 */

#include "QueueSubclass.h"
#include "Job.h"

Define_Module(QueueSubclass);


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
    simtime_t changeStatusTime = par("changeStateDistribution").doubleValue();
    scheduleAt(simTime() + changeStatusTime, wifiStatusMsg);

    EV << "Called INITIALIZE on QueueSubclass\nInitial wifiAvailable = " << wifiAvailable << "\n";
}

void QueueSubclass::handleMessage(cMessage *msg) {
    if (msg == wifiStatusMsg) {
        wifiAvailable = !wifiAvailable;
        EV << "WIFI STATUS CHANGED! Now is " << wifiAvailable << "\n";

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
                    scheduleAt(simTime() + serviceTime, endServiceMsg);
                }
            }
        }
        // wifi ON -> OFF
        else {
            if (servicedJob) {
                suspendService(servicedJob);
            }
            else {
                // Do nothing
            }
        }

        simtime_t changeStatusTime = par("changeStateDistribution").doubleValue();
        scheduleAt(simTime() + changeStatusTime, wifiStatusMsg);
    }
    else if (msg == endServiceMsg) {
        endService(servicedJob);

        if (queue.isEmpty()) {
            servicedJob = nullptr;
            emit(busySignal, false);
        }
        else {
            servicedJob = getFromQueue();
            emit(queueLengthSignal, length());
            simtime_t serviceTime = startService(servicedJob);
            scheduleAt(simTime() + serviceTime, endServiceMsg);
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
                scheduleAt(simTime() + serviceTime, endServiceMsg);
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
}

simtime_t QueueSubclass::startService(Job *job) {
    simtime_t d = simTime() - job->getTimestamp();
    emit(queueingTimeSignal, d);
    job->setTotalQueueingTime(job->getTotalQueueingTime() + d);
    EV << "Starting service of " << job->getName() << endl;
    job->setTimestamp();
    return par("serviceTime").doubleValue();
}

void QueueSubclass::endService(Job *job) {
    EV << "Finishing service of " << job->getName() << endl;
    simtime_t d = simTime() - job->getTimestamp();
    job->setTotalServiceTime(job->getTotalServiceTime() + d);
    send(job, "out");
}

void QueueSubclass::suspendService(Job *job) {
    suspendedJob = job;
    servicedJob = nullptr;
    EV << "Service SUSPENDED! Received: " << job << " - Suspended: " << suspendedJob << " - Serviced: " << servicedJob << endl;
}

void QueueSubclass::resumeService(Job *job) {
    servicedJob = job;
    suspendedJob = nullptr;
    EV << "Service RESUMED! Received: " << job << " - Suspended: " << suspendedJob << " - Serviced: " << servicedJob << endl;
}

void QueueSubclass::finish() {

}



//#include "QueueSubclass.h"
//#include "Queue.h"
//
//
//Define_Module(QueueSubclass);
//
//
//QueueSubclass::QueueSubclass() {
//    jobServiced = nullptr;
//    wifiStatusMsg = nullptr;
//}
//
//QueueSubclass::~QueueSubclass() {
//    delete jobServiced;
//    cancelAndDelete(wifiStatusMsg);
//}
//
//void QueueSubclass::initialize() {
//    Queue::initialize();
//
//    wifiAvailable = true;
//    wifiStatusMsg = new cMessage("wifi_status_changed");
//    simtime_t changeStatusTime = par("changeStateDistribution").doubleValue();
//    scheduleAt(simTime() + changeStatusTime, wifiStatusMsg);
//
//    EV << "Called INITIALIZE on QueueSubclass\nInitial wifiAvailable = " << wifiAvailable << "\n";
//}
//
//void QueueSubclass::arrival(Job *job) {
//    EV << "ARRIVAL for " << job << "\n";
//}
//
//simtime_t QueueSubclass::startService(Job *job) {
//    EV << "START_SERVICE for " << job << "\n";
//    return par("serviceTime").doubleValue();
//}
//
//void QueueSubclass::endService(Job *job) {
//    EV << "END_SERVICE for " << job << "\n";
//    send(job, "out");
//}
//
//void QueueSubclass::handleMessage(cMessage *msg) {
//    if (msg == wifiStatusMsg) {
//        wifiAvailable = !wifiAvailable;
//        simtime_t changeStatusTime = par("changeStateDistribution").doubleValue();
//        scheduleAt(simTime() + changeStatusTime, wifiStatusMsg);
//        EV << "WIFI STATUS CHANGED! Now is " << wifiAvailable << "\n";
//    }
//    else {
//        Job *job = check_and_cast<Job *>(msg);
//        arrival(job);
//
//        if (!servicedJob) {
//            servicedJob = job;
//            emit(busySignal, true);
//
//            simtime_t serviceTime = startService(servicedJob);
//            scheduleAt(simTime() + serviceTime, endServiceMsg);
//        }
//        else {
//            queue.insert(job);
//            emit(queueLengthSignal, length());
//            job->setQueueCount(job->getQueueCount() + 1);
//        }
//    }
//}


