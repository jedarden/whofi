using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using System.Windows;

namespace Panopticon
{
    class GuiInterface : Publisher
    {
        internal volatile MainWindow linkedwindow = null;
        internal volatile Room linkedroom = null;
        internal volatile Boolean killed = false;

        internal void kill()
        {
            killed = true;
        }
        internal override int SupportedOperations
        {
            get
            {
                return (int)DisplayableType.DeviceDevicePosition | (int)DisplayableType.AggregatedStat | (int) DisplayableType.RoomUpdate | (int) DisplayableType.StationUpdate | (int) DisplayableType.DatabaseState | (int) DisplayableType.Rename;
            }
        }

        internal override void publishPosition(Device d, PositionTools.Position p, EventType e)
        {
            if (linkedwindow != null&&!killed)
            {
                if (p.room == linkedroom || e == EventType.Disappear || e == EventType.MoveOut)
                {
                    Application.Current.Dispatcher.BeginInvoke((Action)(() => { if(!killed) linkedwindow.updateDevicePosition(d, p, e); }));
                }
            }
        }

        internal override void publishStat(double stat, Room r, DateTime statTime, StatType s)
        {
            if (linkedwindow!=null && !killed)
            {
                if (s == StatType.OneSecondDeviceCount)
                    Application.Current.Dispatcher.BeginInvoke((Action)(() => { if (!killed) linkedwindow.updateOneSecondDeviceCount(r, stat); }));
                else
                    Application.Current.Dispatcher.BeginInvoke((Action)(() => { if (!killed) linkedwindow.updateFiveMinutesDeviceCount(r, stat); }));
            }
        }
        internal override void publishRoomUpdate(Room r, EventType e)
        {
            if (linkedwindow != null && !killed)
            {
                if (e == EventType.Appear)
                    Application.Current.Dispatcher.BeginInvoke((Action)(() => { if (!killed) linkedwindow.addRoom(r); }));
                else if (e == EventType.Disappear)
                    Application.Current.Dispatcher.BeginInvoke((Action)(() => { if (!killed) linkedwindow.removeRoom(r); }));
            }
            
        }
        internal override void publishStationUpdate(Room r, Station s, EventType e)
        {
            if(linkedwindow!=null && !killed)
                Application.Current.Dispatcher.BeginInvoke((Action)(() => { if (!killed) linkedwindow.updateStation(r,s,e); }));
        }

        internal override void publishDatabaseState(bool v)
        {
            if (linkedwindow != null && !killed)
                Application.Current.Dispatcher.BeginInvoke((Action)(() => { if (!killed) linkedwindow.updateDatabaseState(v); }));
        }
        internal override void publishRename(string oldId, string newId)
        {
            if (linkedwindow != null && !killed)
                Application.Current.Dispatcher.BeginInvoke((Action)(() => { if (!killed) linkedwindow.rename(oldId,newId); }));
        }

    }
}
