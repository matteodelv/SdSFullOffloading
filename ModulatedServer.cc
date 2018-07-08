#include "ModulatedServer.h"
#include "Server.h"
#include "IPassiveQueue.h"

// COPY of Server implementation; custom logic to be implemented
// Added support for multiple output gates

Define_Module(ModulatedServer);

ModulatedServer::ModulatedServer() {
	selectionStrategy = nullptr;
	    jobServiced = nullptr;
	    endServiceMsg = nullptr;
	    allocated = false;
};

ModulatedServer::~ModulatedServer() {
	delete selectionStrategy;
	    delete jobServiced;
	    cancelAndDelete(endServiceMsg);
};

void ModulatedServer::initialize() {
	busySignal = registerSignal("busy");
	    emit(busySignal, false);

	    endServiceMsg = new cMessage("end-service");
	    jobServiced = nullptr;
	    allocated = false;
	    selectionStrategy = SelectionStrategy::create(par("fetchingAlgorithm"), this, true);
	    if (!selectionStrategy)
	        throw cRuntimeError("invalid selection strategy");
};

void ModulatedServer::handleMessage(cMessage *msg) {
	if (msg == endServiceMsg) {
	        ASSERT(jobServiced != nullptr);
	        ASSERT(allocated);
	        simtime_t d = simTime() - endServiceMsg->getSendingTime();
	        jobServiced->setTotalServiceTime(jobServiced->getTotalServiceTime() + d);
	        send(jobServiced, "out", intuniform(0,1));	// intuniform() chooses a random output gate
	        jobServiced = nullptr;
	        allocated = false;
	        emit(busySignal, false);

	        // examine all input queues, and request a new job from a non empty queue
	        int k = selectionStrategy->select();
	        if (k >= 0) {
	            EV << "requesting job from queue " << k << endl;
	            cGate *gate = selectionStrategy->selectableGate(k);
	            check_and_cast<IPassiveQueue *>(gate->getOwnerModule())->request(gate->getIndex());
	        }
	    }
	    else {
	        if (!allocated)
	            error("job arrived, but the sender did not call allocate() previously");
	        if (jobServiced)
	            throw cRuntimeError("a new job arrived while already servicing one");

	        jobServiced = check_and_cast<Job *>(msg);
	        simtime_t serviceTime = par("serviceTime");
	        scheduleAt(simTime()+serviceTime, endServiceMsg);
	        emit(busySignal, true);
	    }
};

void ModulatedServer::refreshDisplay() const {
	getDisplayString().setTagArg("i2", 0, jobServiced ? "status/execute" : "");
};

void ModulatedServer::finish() {

};

bool ModulatedServer::isIdle() {
	return !allocated;
};

void ModulatedServer::allocate() {
	allocated=true;
};
