using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace Panopticon
{
    public class Station
    {
        internal PositionTools.Position location; //contains also Room info
        internal String NameMAC;
        internal DateTime lastHearthbeat;
        internal StationHandler handler=null;
        internal void hearbeat()
        {
            lastHearthbeat = DateTime.Now;
        }
    }
}
