using System;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace Panopticon
{
    abstract class Publisher
    {
        internal enum DisplayableType: int { DeviceDevicePosition=1, SimpleStat=2,Rename=4,SSID=8,RoomUpdate=16,StationUpdate=32, AggregatedStat=64,DatabaseState=128};
        internal class Displayable
        {
            internal Displayable(Object o1, Object o2, Object o3, DisplayableType t)
            {
                arg1 = o1;
                arg2 = o2;
                argtype = o3;
                type = t;
            }
            internal Object arg1;
            internal Object arg2;
            internal Object argtype;
            internal DisplayableType type;
        }
        internal enum EventType { Appear, Update, MoveOut, MoveIn, Disappear };
        internal enum StatType { FiveMinuteAverageDeviceCount, OneSecondDeviceCount, InstantaneousDeviceCount };

        internal abstract int SupportedOperations { get; }
       
        internal bool supportsOperation(DisplayableType dt)
        {
            return ((int)dt & this.SupportedOperations) != 0;
        }
        internal virtual void publishPosition(Device d, PositionTools.Position p, EventType e) { throw new NotSupportedException(); }
        internal virtual void publishStat(double stat, Room r, DateTime statTime, StatType s) { throw new NotSupportedException(); }
        internal virtual void publishRename(String oldId, String newId) { throw new NotSupportedException(); }
        internal virtual void publishRoomUpdate(Room r, EventType e) { throw new NotSupportedException(); }
        internal virtual void publishStationUpdate(Room r, Station s, EventType e) { throw new NotSupportedException(); }

        internal virtual void publishSSID(Device d, String SSID) { throw new NotSupportedException(); }

        internal virtual void publishDatabaseState(bool v) { throw new NotSupportedException(); }
    }
}
