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

network QueueNetwork
{
    submodules:
        sink: Sink {
            @display("p=351,120");
        }
        queue: QueueSubclass {
            @display("p=185,97");
        }
        source: Source {

            @display("p=69,159");
        }
    connections:
        source.out --> queue.in++;
        //queue.out --> {  datarate = 2Mbps; } --> sink.in++;
        queue.out --> sink.in++;
}
