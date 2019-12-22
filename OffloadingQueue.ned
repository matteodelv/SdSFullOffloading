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

import org.omnetpp.queueing.Queue;

//
// A queue with a built-in server
//
simple OffloadingQueue
{
    parameters:
        @group(Queueing);
        @display("i=block/queue;q=queue");
        @signal[dropped](type="long");
        @signal[queueLength](type="long");
        @signal[queueingTime](type="simtime_t");
        @signal[busy](type="bool");
        @statistic[dropped](title="drop event";record=vector?,count?;interpolationmode=none);
        @statistic[queueLength](title="queue length";record=vector?,timeavg?,max?,mean?,count?;interpolationmode=sample-hold);
        @statistic[queueingTime](title="queueing time at dequeue";record=vector?,mean?,max?;unit=s;interpolationmode=none);
        @statistic[busy](title="server busy state";record=vector?,timeavg?;interpolationmode=sample-hold);
        
        @signal[wifiActiveTime](type="simtime_t");
        @statistic[wifiActiveTime](title="time in which wifi connection was available";record=vector,mean?;unit=s;interpolationmode=none);
        
        @signal[cellActiveTime](type="simtime_t");
        @statistic[cellActiveTime](title="time in which wifi connection was not available (falling back on cellular)";record=vector,mean?;unit=s;interpolationmode=none);
        
        @signal[deadlineDistrib](type="simtime_t");
        @statistic[deadlineDistrib](title="time in which the server was connected to cellular";record=vector,mean?;unit=s;interpolationmode=none);
        
        @signal[jobServiceTime](type="simtime_t");
        @statistic[jobServiceTime](title="time in which jobs are offloaded";record=vector;unit=s;interpolationmode=none);

        int capacity = default(-1);    // negative capacity means unlimited queue
        bool fifo = default(true);     // whether the module works as a queue (fifo=true) or a stack (fifo=false)
        volatile double serviceTime @unit(s);
        
        volatile double deadlineDistribution @unit(s);
        volatile double wifiStateDistribution @unit(s);
        volatile double cellularStateDistribution @unit(s);
    gates:
        input in[];
        output out[2];
}
