using System;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;

namespace Panopticon
{
    class DatabasePublisher:Publisher
    {
        DatabaseInterface DBInt;
        volatile Boolean killed = false;
        Int64 ongoingqueryes = 0;
        public DatabasePublisher(DatabaseInterface di)
        {
            DBInt = di;
        }

        internal override int SupportedOperations
        {
            get
            {
                return (int)DisplayableType.DeviceDevicePosition | (int)DisplayableType.Rename | (int)DisplayableType.SSID | (int)DisplayableType.AggregatedStat;
            }
        }
        internal override void publishPosition(Device d, PositionTools.Position p, EventType e)
        {
            Interlocked.Increment(ref ongoingqueryes);
            if (!killed)
            {
                Task.Factory.StartNew(() =>
                {
                    DBInt.addDevicePosition(d.identifier, d.MAC, p.room.roomName, p.X, p.Y, p.uncertainity, p.positionDate, e);
                    Interlocked.Decrement(ref ongoingqueryes);
                });
            }
            else
                Interlocked.Decrement(ref ongoingqueryes);
        }
        internal override void publishStat(double stat, Room r, DateTime statTime, StatType s)
        {
            Interlocked.Increment(ref ongoingqueryes);
            if (!killed)
            {
                Task.Factory.StartNew(() =>
                {
                    if (s == StatType.OneSecondDeviceCount)
                    {
                        DBInt.updateRoomCount(stat, r.roomName);
                        DBInt.addLTRoomCount(stat, r.roomName, statTime, 2);
                    }
                    else
                        DBInt.addLTRoomCount(stat, r.roomName, statTime, 1);
                    Interlocked.Decrement(ref ongoingqueryes);
                });
            }
            else
                Interlocked.Decrement(ref ongoingqueryes);
        }
        internal override void publishRename(String oldId, String newId)
        {
            Interlocked.Increment(ref ongoingqueryes);
            if (!killed)
            {
                Task.Factory.StartNew(() =>
                {
                    DBInt.renameDevice(oldId, newId);
                    Interlocked.Decrement(ref ongoingqueryes);
                });
            }
            else
                Interlocked.Decrement(ref ongoingqueryes);
        }

        internal override void publishSSID(Device d, String SSID)
        {
            Interlocked.Increment(ref ongoingqueryes);
            if (!killed)
            {
                Task.Factory.StartNew(() =>
                {
                    DBInt.addRequestedSSID(d.identifier, SSID);
                    Interlocked.Decrement(ref ongoingqueryes);
                });
            }
            else
                Interlocked.Decrement(ref ongoingqueryes);

        }

        internal void kill()
        {
            killed = true;
        }

        internal void confirmclose()
        {
            // this check should always be confirmed at the first attempt, therefore a busy wait is perfectly fine,
            // anything more (like a ConditionVariable signaled every time the counter is 0) is just a waste of performance and code.
            // if is > 0 there could be still a query running, so cannot confirm.
            // if = 0 no query running, and even a new request of query would be blocked by the volatile killed. 
            while (Interlocked.Read(ref ongoingqueryes) > 0) ;
        }

        
    }
}
