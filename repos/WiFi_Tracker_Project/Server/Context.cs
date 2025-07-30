using System;
using System.Collections.Generic;
using System.Collections;
using System.Linq;
using System.Runtime.CompilerServices;
using System.Text;
using System.Threading.Tasks;
using System.Collections.Concurrent;
using System.Threading;
using System.Windows;

namespace Panopticon
{
    class Context
    {
        ConcurrentDictionary<String, Station> stations;
        ConcurrentDictionary<String, Room> rooms;
        ConcurrentDictionaryStack<String, Device> deviceMap;
        AnalysisEngine analyzer;
        ReaderWriterLockSlim locker = new ReaderWriterLockSlim();
        DatabasePublisher databasePub;
        internal readonly DatabaseInterface databaseInt;
        List<Publisher> publishers;
        Aggregator aggregator;
		Connection connection;
		LinkedList<Thread> threads = new LinkedList<Thread>();
        internal readonly GuiInterface guiPub;
		
        public Context()
        {
            stations = new ConcurrentDictionary<String, Station>();
            deviceMap = new ConcurrentDictionaryStack<String, Device>();
            rooms = new ConcurrentDictionary<String, Room>();
            try
            {
                databaseInt = new DatabaseInterface(Properties.Settings.Default.ConnectionString);
            }
            catch
            {
                System.Environment.Exit(1);
                return;
            }
            publishers = new List<Publisher>();
            databasePub = new DatabasePublisher(databaseInt);
            publishers.Add(databasePub);
            guiPub = new GuiInterface();
            databaseInt.attachStateListener(guiPub);
            publishers.Add(guiPub);
            aggregator = new Aggregator(publishers);
            publishers.Add(aggregator);
            analyzer = new AnalysisEngine(publishers, deviceMap);
			connection = new Connection(this);
		}
        /// <summary>
        /// Initialize and start all the background threads of the application
        /// </summary>
        public void orchestrate()
        {
            Thread socketListenerT = new Thread(new ThreadStart(connection.StartConnection));
			socketListenerT.Name = "Server thread";
			socketListenerT.Start();
			Thread analyzerT = new Thread(new ThreadStart(analyzer.analyzerProcess));
            analyzerT.Name = "Analyzer thread";
            analyzerT.Start();
            Thread aggregatorT = new Thread(new ThreadStart(aggregator.aggregatorProcess));
            aggregatorT.Name = "Aggregator thread";
            aggregatorT.Start();
			threads.AddLast(socketListenerT);
			threads.AddLast(analyzerT);
            threads.AddLast(aggregatorT);
		}
        /// <summary>
        /// Get the current analysis engine
        /// </summary>
        public AnalysisEngine getAnalyzer()
        {
           return analyzer;
        }
       /// <summary>
       /// Associate a station, if its config exists, or open the config procedure
       /// </summary>
        public Station tryAddStation(String NameMAC, StationHandler _handler, bool AllowAsynchronous) //Replace Object with the relevant type
        {
            //check if not attempting reconnection
            locker.EnterReadLock(); //lock to avoid removal if reconnecting
            Station s = getStation(NameMAC);
            if(s!=null) // s is reconnecting after losing contact
            {
				s.handler.socket.Close();
				s.handler = _handler; //updating handler
                s.hearbeat();
                locker.ExitReadLock();
                return s;
            }
            locker.ExitReadLock();
            s = loadStation(NameMAC, _handler); //search Station in DB
            if (s==null && AllowAsynchronous) //this is already the check if a configuration for the station exists or not
            {
				int? wId = FindWindowByMAC(_handler.macAddress);
				if (wId != null)
				{
					int id = wId ?? 0;
					Application.Current.Windows[id].Close();
				}
				StationAdder sa1 = new StationAdder(this, _handler);
				sa1.Show();
				return null;
            }
            return s;
        }

		internal int? FindWindowByMAC(string _macAddress)
		{
			int i = 0;
			foreach (Window w in Application.Current.Windows)
			{
				if (String.Compare((string)(w.Tag), _macAddress) == 0)
				{
					return i;
				}
				else i++;
			}
			return null;
		}
        /// <summary>
        /// Load the rooms from persistent storage
        /// </summary>
		public void loadRooms()
        {
            foreach (DatabaseInterface.RoomInfo ri in databaseInt.loadRooms())
                createRoom(new Room(ri.RoomName, ri.Xlen, ri.Ylen));
            createRoom(Room.externRoom);
            return;
        }
        /// <summary>
        /// Check if a name is already used by a room in persistent storage
        /// </summary>
        public Nullable<bool> checkRoomExistence(String roomName)
        {
            bool? val = databaseInt.checkRoomExistence(roomName);
            if(val.HasValue)
               val = new bool?(val.Value|| roomName == "External" || roomName == "Overall");
            return val;
        }
        /// <summary>
        /// Save room to persistent storage
        /// </summary>
        public bool saveRoom(Room room)
        {
            return databaseInt.saveRoom(room.roomName,room.size.X,room.size.Y);
        }
        /// <summary>
        /// Mark room in persistent storage as archived
        /// </summary>
        public bool archiveRoom(String RoomName)
        {
            return databaseInt.archiveRoom(RoomName);
        }
        /// <summary>
        /// Try to load the configs of a station from persistent storage
        /// </summary>
        public Station loadStation(String NameMAC, StationHandler handler)
        {
            DatabaseInterface.StationInfo? si = databaseInt.loadStationInfo(NameMAC);
            if (si == null)
                return null;
            Station s=new Station();
            s.hearbeat();
            s.NameMAC = NameMAC;
            s.handler = handler;
            String roomName=si.Value.RoomName;
            double x=si.Value.X;
            double y=si.Value.Y;
            Room room=getRoom(roomName);
            if(room == null) // The room to which it was attached does not exist anymore. Reconfig
            {
                deleteStation(NameMAC);
                return null;
            }
            s.location = new PositionTools.Position(x,y,room);
            locker.EnterWriteLock();
            room.addStation(s);
            stations[s.NameMAC] = s;
            locker.ExitWriteLock();
            foreach (Publisher pb in publishers)
                if (pb.supportsOperation(Publisher.DisplayableType.StationUpdate))
                    pb.publishStationUpdate(room,s,Publisher.EventType.Appear);
            return s;
        }
        /// <summary>
        /// Save station config to persistent storage
        /// </summary>
        public bool saveStation(Station s)
        {
            DatabaseInterface.StationInfo si = new DatabaseInterface.StationInfo();
            si.NameMAC = s.NameMAC;
            si.RoomName = s.location.room.roomName;
            si.X = s.location.X;
            si.Y = s.location.Y;
            return databaseInt.saveStationInfo(si); ;
        }
        /// <summary>
        /// Remove the config of a station fro persisten storage
        /// </summary>
        public bool deleteStation(String NameMAC)
        {
            return databaseInt.removeStation(NameMAC);
        }
        /// <summary>
        /// Create a new station object for a room
        /// </summary>
        public Station createStation(Room room, String NameMAC, double X, double Y, StationHandler handler)
        {
            Station s = new Station();
            s.lastHearthbeat = DateTime.Now;
            s.NameMAC = NameMAC;
            s.location = new PositionTools.Position(X, Y, room);
            s.handler = handler;
            locker.EnterWriteLock();
            room.addStation(s);
            stations[s.NameMAC] = s;
            locker.ExitWriteLock();
            foreach (Publisher pb in publishers)
                if (pb.supportsOperation(Publisher.DisplayableType.StationUpdate))
                    pb.publishStationUpdate(room,s,Publisher.EventType.Appear);
            return s;
        }
        /// <summary>
        /// Return the object instance for a currently associated station
        /// </summary>
        public Station getStation(String NameMAC)
        {
            return stations.ContainsKey(NameMAC)?stations[NameMAC]:null;
        }
        /// <summary>
        /// Check if a station has been succesgully associated
        /// </summary>
		public bool StationConfigured(String NameMAC)
		{
			if (stations.ContainsKey(NameMAC))
				return true;
			else return false;
		}
        /// <summary>
        /// Get all the rooms loaded
        /// </summary>
        public IEnumerable<Room> getRooms()
        {
            return rooms.Values.ToArray<Room>();
        }
        /// <summary>
        /// Get the object instance for a specific room
        /// </summary>
        public Room getRoom(String name)
        {
            Room room;
            return rooms.TryGetValue(name, out room)?room:null;
        }
        /// <summary>
        /// Deassociate a station
        /// </summary>
        public void removeStation(String NameMAC, bool takelock=true)
        {
            Station s;
			stations.TryRemove(NameMAC, out s);
            Room room=s.location.room;
            if(takelock)
                locker.EnterWriteLock();
            room.removeStation(s);
            if(takelock)
                locker.ExitWriteLock();
            foreach (Publisher pb in publishers)
                if (pb.supportsOperation(Publisher.DisplayableType.StationUpdate))
                    pb.publishStationUpdate(room,s,Publisher.EventType.Disappear);
        }
        /// <summary>
        /// Unload a room
        /// </summary>
        public void removeRoom(String RoomName)
        {
            Room room = rooms[RoomName];
            locker.EnterWriteLock();
            foreach (Station s in room.getStations())
            {
                removeStation(s.NameMAC, false);
                deleteStation(s.NameMAC);
				s.handler.reboot();
            }
            rooms.TryRemove(room.roomName, out room);
            locker.ExitWriteLock();
            foreach (Publisher pb in publishers)
                if (pb.supportsOperation(Publisher.DisplayableType.RoomUpdate))
                    pb.publishRoomUpdate(room, Publisher.EventType.Disappear);
        }
        /// <summary>
        /// Verify the status of associated stations in a room
        /// </summary>
        public bool checkStationAliveness(Room room)
        {
            bool ris = true;
            locker.EnterUpgradeableReadLock();
            foreach(Station s in room.getStations())
            {
                if (s.lastHearthbeat.AddMinutes(1.1) < DateTime.Now) 
                {
                    locker.EnterWriteLock();
                    if(s.lastHearthbeat.AddMinutes(1.1) < DateTime.Now) // check again: was in readmode, may have reconnected now and updated
                    {
                        ris = false;
                        removeStation(s.NameMAC, false);
                    }
                    locker.ExitWriteLock();
                }
            }
            locker.ExitUpgradeableReadLock();
            return ris;
        }
        /// <summary>
        /// Verify the status of all associated stations
        /// </summary>
		public void checkAllStationAliveness()
		{
			foreach (Room r in rooms.Values)
			{
				if (!checkStationAliveness(r))
				{
					System.Console.WriteLine("Station de-connetted in room {0}", r.roomName);
				}
			}
		}
        /// <summary>
        /// Load a new room
        /// </summary>
        public Room createRoom(Room r)
        {
            //init station per room, user per room
            locker.EnterWriteLock();
            if (rooms.ContainsKey(r.roomName))
                r = null;
            else
            {
                rooms[r.roomName] = r;
            }
            locker.ExitWriteLock();
            if(r!=null)
                foreach (Publisher pb in publishers)
                    if (pb.supportsOperation(Publisher.DisplayableType.RoomUpdate))
                        pb.publishRoomUpdate(r,Publisher.EventType.Appear);
            return r;
        }
        /// <summary>
        /// Orderly release resources and shutdown
        /// </summary>
        public void kill()
        {
			connection.kill();
			MainWindow wx = guiPub.linkedwindow;
            wx.kill();
            guiPub.kill();
            analyzer.kill();
            aggregator.kill();
            databasePub.kill();
            foreach(Thread t in threads)
            {
                t.Join();
            }
            foreach (Room r in getRooms())
            {
                foreach (Device d in r.getDevices())
                    databaseInt.addDevicePosition(d.identifier, d.MAC, r.roomName, 0, 0, 0, DateTime.Now, Publisher.EventType.Disappear); //sync push on db
                foreach (Station s in r.getStations())
                    s.handler.reboot(false);
            }

			//closing configuration prompts, if opened
			foreach (Window w in Application.Current.Windows)
			{
				if (w.Tag != null)
					w.Close();
			}

            wx.confirmclose();
            databasePub.confirmclose(); // wait that everything has been written to the db before killing.
            databaseInt.close(); //wait that no requests is in flight before killing the db
            Environment.Exit(0);
        }


    }
}
