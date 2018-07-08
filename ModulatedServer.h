/*
 * ModulatedServer.h
 *
 *  Created on: 07 lug 2018
 *      Author: matteo
 */

#ifndef MODULATEDSERVER_H_
#define MODULATEDSERVER_H_

#include <omnetpp.h>
#include "IServer.h"
#include "Job.h"
#include "SelectionStrategies.h"

using namespace queueing;

// COPY of Server implemetation; custom logic to be implemented
// Added support for multiple output gates in the NED file

class ModulatedServer : public cSimpleModule, public queueing::IServer {

private:
        simsignal_t busySignal;
        bool allocated;

        SelectionStrategy *selectionStrategy;

        Job *jobServiced;
        cMessage *endServiceMsg;

    public:
        ModulatedServer();
        virtual ~ModulatedServer();

    protected:
        virtual void initialize() override;
        virtual int numInitStages() const override {return 2;}
        virtual void handleMessage(cMessage *msg) override;
        virtual void refreshDisplay() const override;
        virtual void finish() override;

    public:
        virtual bool isIdle() override;
        virtual void allocate() override;
};


#endif /* MODULATEDSERVER_H_ */
