using System;
using System.Collections.Generic;
using System.Linq;
using System.Net.Sockets;
using System.Text;
using System.Threading.Tasks;

namespace Panopticon
{
    class StationHandler
    {
        internal Socket socket;
		internal string macAddress = null;
        internal bool isBlinking = false;
        internal bool isOn = true;
        internal StationHandler(Socket _socket, string _macAddress, Context ctx)
        {
            socket = _socket;
			macAddress = _macAddress;
        }

        internal void switchLedBlink(bool blink)
        {
			if (blink == false)
			{
				Protocol.ESP_BlinkStop(socket);
				isBlinking = false;
			}
			else
			{
				Protocol.ESP_BlinkStart(socket);
				isBlinking = true;
			}    
        }

        internal void reboot(bool asynchronous=true)
        {
            if(asynchronous)
            {
                Task.Factory.StartNew(() =>
                {
                    Protocol.ESP_Reboot(socket);
                });
            }
            else
            {
                Protocol.ESP_Reboot(socket);
            }

        }

        internal void shutdown()
        {
            if (!isOn)
                Protocol.ESP_StandBy(socket);
            else
                Protocol.ESP_Resume(socket);
        }
    }
}
