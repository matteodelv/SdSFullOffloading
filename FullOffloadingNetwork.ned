//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
//

import org.omnetpp.queueing.Source;
import org.omnetpp.queueing.Queue;
import org.omnetpp.queueing.Sink;
import org.omnetpp.queueing.PassiveQueue;
import org.omnetpp.queueing.Server;


network FullOffloadingNetwork
{
    parameters:
        @display("i=block/network2");

        int meanJobSize @unit(byte) = default(1MB);
        volatile double wifiAvailableDuration @unit(s) = default(20min);
        volatile double cellularAvailableDuration @unit(s) = default(40min);
        double wifiDataRate @unit(bps) = default(20Mbps);
        double cellularDataRate @unit(bps) = default(5Mbps);
        double wifiPowerConsumption @unit(W) = default(1W);
        double cellularPowerConsumption @unit(W) = default(3W);
        double renegingRate = default(0.15);
    submodules:
        sink: Sink {
            @display("p=454,173");
        }
        wifiQueue: PassiveQueue {
            @display("i=block/passiveq;p=125,118");
        }
        wifiServer: ModulatedServer {
            @display("i=block/server;p=188,185");
        }
        cellularQueue: Queue {
            @display("i=block/queue;p=227,73");
        }
        remoteQueue: Queue {
            @display("i=block/queue");
        }
        source: Source {
            @display("p=42,168");
        }
    connections:
        source.out --> wifiQueue.in++;
        wifiQueue.out++ --> wifiServer.in++;
        wifiServer.out++ --> cellularQueue.in++;
        wifiServer.out++ --> {  datarate = wifiDataRate; } --> remoteQueue.in++;
        cellularQueue.out --> {  datarate = cellularDataRate; } --> remoteQueue.in++;
        remoteQueue.out --> sink.in++;
}
