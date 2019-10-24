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
import org.omnetpp.queueing.Sink;
import org.omnetpp.queueing.Queue;

network QueueNetwork
{
    submodules:
        sink: Sink {
            @display("p=364,167");
        }
        wifiQueue: QueueSubclass {
            @display("p=140,136");
        }
        cellularQueue: Queue {
            @display("p=259,57");
        }
        remoteQueue: Queue {
            @display("p=259,187");
        }
        source: Source {
            @display("p=50,72");
        }
    connections:
        source.out --> wifiQueue.in++;
        //queue.out --> {  datarate = 2Mbps; } --> sink.in++;
        wifiQueue.out[1] --> remoteQueue.in++;
        wifiQueue.out[0] --> cellularQueue.in++;
        cellularQueue.out --> remoteQueue.in++;
        remoteQueue.out --> sink.in++;

}
