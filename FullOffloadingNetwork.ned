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


network FullOffloadingNetwork
{
    parameters:
        @display("i=block/network2");
    submodules:
        sink: Sink {
            @display("p=451,196");
        }
        wifiQueue: Queue {
            @display("p=183,255");
        }
        cellularQueue: Queue {
            @display("i=queue");
        }
        remoteQueue: Queue {
            @display("i=queue;");
        }
        source: Source {
            @display("p=52,196");
        }
    connections:
        source.out --> wifiQueue.in++;
        source.out --> cellularQueue.in++;
        remoteQueue.out --> sink.in++;
        wifiQueue.out --> cellularQueue.in++;
        wifiQueue.out --> remoteQueue.in++;
        cellularQueue.out --> remoteQueue.in++;
}
