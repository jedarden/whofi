using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace Panopticon
{
    class Packet
    {
        internal class Reception
        {
            internal Station ReceivingStation;
            internal double RSSI;
            internal Reception(Station receivingStation, double rssi)
            {
                ReceivingStation = receivingStation;
                RSSI = rssi;
            }
        }
        internal List<Reception> Receivings = new List<Reception>();
        internal Packet(String _sendingMAC, String _requestedSSID, DateTime _timestamp, String _hash, String _htcapabilities, Int64 _sequenceNumber)
        {
            SendingMAC = _sendingMAC;
            RequestedSSID = _requestedSSID;
            Timestamp = _timestamp;
            Hash = _hash;
            HTCapabilities = _htcapabilities;
            SequenceNumber = _sequenceNumber;
        }
        internal void received(Station ReceivingStation, Double RSSI)
        {
            Receivings.Add(new Reception(ReceivingStation, RSSI));
        }
        internal String SendingMAC;
        internal String RequestedSSID; //null if no requested SSID
        internal DateTime Timestamp;
        internal String Hash;
        internal String HTCapabilities;
        internal Int64 SequenceNumber;
    }
}
