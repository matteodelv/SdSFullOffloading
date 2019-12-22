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

import org.omnetpp.queueing.Sink;

network FullOffloadingNetwork
{
    @display("bgb=601,359");
    submodules:
        sink: LimitedSink {
            @display("p=492,169");
        }
        wifiQueue: OffloadingQueue {
            @display("p=171,199");
        }
        cellularQueue: QueueCustom {
            @display("p=316,97");
        }
        remoteQueue: QueueCustom {
            @display("p=343,269");
        }
        source: LimitedSource {
            @display("p=84,97");
        }
    connections:
        source.out --> wifiQueue.in++;
        wifiQueue.out[1] --> remoteQueue.in++;
        wifiQueue.out[0] --> cellularQueue.in++;
        cellularQueue.out --> remoteQueue.in++;
        remoteQueue.out --> sink.in++;

}
