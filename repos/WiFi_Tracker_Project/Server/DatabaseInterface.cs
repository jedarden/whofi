using Npgsql;
using System;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.Globalization;
using System.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using System.Windows;

namespace Panopticon
{
    class DatabaseInterface
    {
        private const int CONNPOOLSIZE = 4;
        private volatile int criticalstate = 0;
        private readonly String connectionstring;
        BlockingCollection<NpgsqlConnection> connectionpool = new BlockingCollection<NpgsqlConnection>();
        private volatile Publisher dbstatelistener = null;

        internal class ConnectionHandle : IDisposable
        {
            private BlockingCollection<NpgsqlConnection> cp;
            internal NpgsqlConnection conn;
            internal enum ConnectionState { Recovered, Open, Broken };
            internal ConnectionState state = ConnectionState.Open;
            internal ConnectionHandle(BlockingCollection<NpgsqlConnection> connectionpool, String connectionstring, bool recover=true)
            {
                cp = connectionpool;
                conn = cp.Take();
                if(conn.State==System.Data.ConnectionState.Broken||conn.State==System.Data.ConnectionState.Closed)
                {
                    if (recover)
                    {
                        try
                        {
                            conn = new NpgsqlConnection(connectionstring);
                            conn.Open();
                            state = ConnectionState.Recovered;
                        }
                        catch (Exception)
                        {
                            state = ConnectionState.Broken;
                        }
                    }
                    else
                        state = ConnectionState.Broken;
                }
            }
            public void Dispose()
            {
                cp.Add(conn);
            }
        }
        internal struct StationInfo
        {
            internal String NameMAC;
            internal String RoomName;
            internal Double X;
            internal Double Y;
        }
        internal struct RoomInfo
        {
            internal String RoomName;
            internal double Xlen;
            internal double Ylen;
        }
        public DatabaseInterface(String _connectionstring) 
        {
            connectionstring = _connectionstring;
            try
            {
                for(int i=0;i<CONNPOOLSIZE;i++)
                {
                    NpgsqlConnection conn = new NpgsqlConnection(connectionstring);
                    conn.Open();
                    connectionpool.Add(conn);
                }
            }
            catch(Exception ex)
            {
                MessageBox.Show(ex.Message);
                throw new Exception(ex.Message);
            }
            
        }
        internal void attachStateListener(Publisher pub)
        {
            dbstatelistener = pub;
        }
        internal void close()
        {
            foreach (NpgsqlConnection conn in connectionpool.ToArray())
            {
                conn.Close();
            }
        }
        private void manageDbException(Exception ex, NpgsqlConnection conn, Boolean userfacing)
        {
            bool connectionerror = conn.State==System.Data.ConnectionState.Broken||conn.State==System.Data.ConnectionState.Closed;
            if (userfacing || (connectionerror && Interlocked.CompareExchange(ref criticalstate, 1, 0) == 0))
            {
                if (connectionerror)
                {
                    if (conn.State != System.Data.ConnectionState.Closed)
                        conn.Close();
                    dbstatelistener.publishDatabaseState(false);
                    MessageBox.Show("Connection with the database broken. Will reconnect automatically when database returns available, however meanwhile data will be lost.");
                }
                else
                    MessageBox.Show(ex.Message);
            }
        }

        private bool connectioncheck(ConnectionHandle conn)
        {
            if((conn.state==ConnectionHandle.ConnectionState.Recovered)&&Interlocked.CompareExchange(ref criticalstate,0,1)==1)
            {
                dbstatelistener.publishDatabaseState(true);
            }
            else if(conn.state==ConnectionHandle.ConnectionState.Broken)
            {
                manageDbException(new Exception(),conn.conn,false);
                return false;
            }
            return true;
        }
        
        private bool performNonQuery(String sql, params object[] parameters)
        {
            int res = 0;
            using (ConnectionHandle conn = new ConnectionHandle(connectionpool,connectionstring))
            {
                if (!connectioncheck(conn))
                    return false;
                try
                {
                    using (var cmd = new NpgsqlCommand(sql, conn.conn))
                    {
                        for (int i = 0; i < parameters.Length; i += 2)
                            cmd.Parameters.AddWithValue((String)parameters[i], parameters[i + 1]);
                        res = cmd.ExecuteNonQuery();                
                    }
                }
                catch (Npgsql.PostgresException ex)
                {
                    if (ex.SqlState == "23505") //Sql state for unique constraint violation
                        res = -1;
                    else
                    {
                        manageDbException(ex, conn.conn, false);
                        res = -2;
                    }
                }
                catch (Exception ex)
                {
                    manageDbException(ex, conn.conn, false);
                    res = -2;
                }
            }
            return res >= 0;
        }

        private void addParameters(NpgsqlCommand cmd,params object[] parameters)
        {
            for (int i = 0; i < parameters.Length; i += 2)
                cmd.Parameters.AddWithValue((String)parameters[i], parameters[i + 1]);
        }

        internal Nullable<StationInfo> loadStationInfo(String NameMAC)
        {
            Nullable<StationInfo> si = null;
            using (ConnectionHandle conn = new ConnectionHandle(connectionpool,connectionstring))
            {
                String query = "select roomname,xpos,ypos from stations where namemac=@namemac";
                try
                {
                    if (!connectioncheck(conn))
                        throw new Exception("Connection is not open");
                    using (var cmd = new NpgsqlCommand(query, conn.conn))
                    {
                        cmd.Parameters.AddWithValue("namemac", NameMAC);
                        using (var reader = cmd.ExecuteReader())
                        {
                            if (reader.Read())
                            {
                                StationInfo si2 = new StationInfo();
                                si2.NameMAC = NameMAC;
                                si2.RoomName = reader.GetString(0);
                                si2.X = reader.GetFloat(1);
                                si2.Y = reader.GetFloat(2);
                                si = si2;
                            }
                            else
                                si = null;
                        }
                    }
                }
                catch (Exception ex)
                {
                    manageDbException(ex, conn.conn, true);
                    si = null;
                }
            }
            return si;
        }

        internal Nullable<Boolean> checkRoomExistence(String roomName)
        {
            using (ConnectionHandle conn = new ConnectionHandle(connectionpool,connectionstring))
            {
                String query = "select * from rooms where roomname=@roomname";
                try
                {
                    if (!connectioncheck(conn))
                        throw new Exception("Connection is not open");
                    using (var cmd = new NpgsqlCommand(query, conn.conn))
                    {
                        addParameters(cmd,"roomname", roomName);
                        using (var reader = cmd.ExecuteReader())
                            return new bool?(reader.Read());
                    }
                }
                catch(Exception ex)
                {
                    manageDbException(ex, conn.conn, true);
                    return null; //most conservative result
                }
            }

        }

        internal IEnumerable<RoomInfo> loadRooms()
        {
            LinkedList<RoomInfo> li = new LinkedList<RoomInfo>();
            using (ConnectionHandle conn = new ConnectionHandle(connectionpool,connectionstring))
            {
                String query = "select roomname,xlength,ylength from rooms where archived=false";
                try
                {
                    if (!connectioncheck(conn))
                        throw new Exception("Connection is not open");
                    using (var cmd = new NpgsqlCommand(query, conn.conn))
                    {
                        using (var reader = cmd.ExecuteReader())
                        {
                            while (reader.Read())
                            {
                                RoomInfo ri = new RoomInfo();
                                ri.RoomName = reader.GetString(0);
                                ri.Xlen = reader.GetFloat(1);
                                ri.Ylen = reader.GetFloat(2);
                                li.AddLast(ri);
                            }
                        }

                    }
                }
                catch (Exception ex)
                {
                    manageDbException(ex, conn.conn, true);
                    li = new LinkedList<RoomInfo>();
                }
            }
            return li;
        }

        internal bool saveStationInfo(StationInfo si)
        {
			return performNonQuery("insert into stations(namemac,roomname,xpos,ypos) values (@namemac,@roomname,@xpos,@ypos)",
                "namemac",si.NameMAC,
                "roomname", si.RoomName,
                "xpos",si.X,
                "ypos",si.Y);
        }        

        internal bool saveRoom(String RoomName, double Xlen, double Ylen)
        {
           return performNonQuery("insert into rooms(roomname,xlength,ylength,archived) values (@roomname,@xlength,@ylength,false)",
                "roomname",RoomName,
                "xlength",Xlen,
                "ylength",Ylen);
        }

        internal bool archiveRoom(string roomName)
        {
            return performNonQuery("update rooms set archived=true where roomname=@roomname",
                "roomname",roomName);
        }

        internal bool removeStation(string nameMAC)
        {
            return performNonQuery("delete from stations where namemac=@namemac",
                "namemac",nameMAC);
        }

        internal bool updateRoomCount(double count, String roomName)
        {
            return performNonQuery("update rooms set pcount=@count where roomname=@roomname",
                "count",count,
                "roomname",roomName);
        }

        internal bool addLTRoomCount(double stat, String roomname, DateTime timestamp, int cat)
        {
            return performNonQuery("insert into countstats(count,roomname,tm,xhour,xday,xmonth,xyear,cat) values(@count,@roomname,@tm,@xhour,@xday,@xmonth,@xyear,@cat)",
                "count",stat,
                "roomname",roomname,
                "tm",timestamp,
                "xhour",timestamp.Hour,
                "xday",timestamp.Day,
                "xmonth",timestamp.Month,
                "xyear",timestamp.Year,
                "cat",cat);
        }

        internal bool addRequestedSSID(string identifier, string SSID)
        {
            return performNonQuery("insert into requestedssids(identifier,ssid) values(@identifier,@ssid)",
                "identifier",identifier,
                "ssid",SSID);
        }

        internal bool renameDevice(string oldid, string newid)
        {
            bool res = performNonQuery("update requestedssids set identifier=@newid where identifier=@oldid",
                "newid",newid,
                "oldid",oldid);
            return res && performNonQuery("update devicespositions set identifier=@newid where identifier=@oldid",
                "newid",newid,
                "oldid",oldid);
        }
        internal bool addDevicePosition(string identifier, string mac, string roomname, double xpos, double ypos, double uncertainity, DateTime timestamp, Publisher.EventType evty)
        {
            return performNonQuery("insert into devicespositions(identifier,mac,roomname,tm,xpos,ypos,uncertainty,outmovement,xhour,xday,xmonth,xyear) values(@identifier,@mac,@roomname,@tm,@xpos,@ypos,@uncertainty,@outmovement,@xhour,@xday,@xmonth,@xyear)",
                "identifier",identifier,
                "mac",mac,
                "roomname",roomname,
                "tm",timestamp,
                "xpos",xpos,
                "ypos",ypos,
                "uncertainty",uncertainity,
                "outmovement", (evty == Publisher.EventType.Disappear || evty == Publisher.EventType.MoveOut),
                "xhour",timestamp.Hour,
                "xday",timestamp.Day,
                "xmonth",timestamp.Month,
                "xyear",timestamp.Year);
        }
        internal struct DevicePosition
        {
            internal String identifier;
            internal double xpos;
            internal double ypos;
            internal double prexpos;
            internal double preypos;
            internal double uncertainity;
            internal DateTime timestamp;
            internal bool moveout;
        }
        internal DevicePosition[] loadDevicesPositions(string roomName, DateTime fromdate, string fromtime, DateTime todate, string totime)
        {
            LinkedList<DevicePosition> li = new LinkedList<DevicePosition>();
            Dictionary<String, DevicePosition> prepos = new Dictionary<String, DevicePosition>();
            using (ConnectionHandle conn = new ConnectionHandle(connectionpool,connectionstring,false))
            {
                String query = "select identifier,xpos,ypos,tm,outmovement,uncertainty from devicespositions where roomname=@roomname and tm>=@tmstart and tm<=@tmend order by tm asc";
                try
                {
                    if (!connectioncheck(conn))
                        throw new Exception("Connection is not open");
                    using (var cmd = new NpgsqlCommand(query, conn.conn))
                    {
                        addParameters(cmd,
                            "roomname", roomName,
                            "tmstart", new DateTime(fromdate.Year, fromdate.Month, fromdate.Day, Convert.ToInt32(fromtime.Split(':')[0]), Convert.ToInt32(fromtime.Split(':')[1]), 0),
                            "tmend", new DateTime(todate.Year, todate.Month, todate.Day, Convert.ToInt32(totime.Split(':')[0]), Convert.ToInt32(totime.Split(':')[1]), 0));
                        using (var reader = cmd.ExecuteReader())
                        {
                            while (reader.Read())
                            {
                                DevicePosition dp = new DevicePosition();
                                DevicePosition dp2;
                                dp.identifier = reader.GetString(0);
                                dp.xpos = reader.GetDouble(1);
                                dp.ypos = reader.GetDouble(2);
                                dp.timestamp = reader.GetDateTime(3);
                                dp.moveout = reader.GetBoolean(4);
                                dp.uncertainity = reader.GetDouble(5);
                                if (prepos.TryGetValue(dp.identifier, out dp2))
                                {
                                    dp.prexpos = dp2.xpos;
                                    dp.preypos = dp2.ypos;
                                    prepos[dp.identifier] = dp;
                                }
                                else
                                {
                                    dp.prexpos = -1;
                                    dp.preypos = -1;
                                    prepos.Add(dp.identifier, dp);
                                }
                                if (dp.moveout)
                                    prepos.Remove(dp.identifier);
                                li.AddLast(dp);
                            }
                        }
                    }
                }
                catch (Exception ex)
                {
                    manageDbException(ex, conn.conn, true);
                    return null;
                }
            }
            return li.ToArray<DevicePosition>();
        }

        private int dayspermonth(int month, int year)
        {
            int numdays = 31;
            if (month == 2)
            {
                numdays = 28;
                if (year % 4 == 0 && (year % 100 != 0 || year % 1000 == 0))
                    numdays = 29;
            }
            else if (month == 4 || month == 6 || month == 9 || month == 11)
                numdays = 30;
            return numdays;
        }

        internal double[] loadMaxDevicesDay(int selectedmonth, int selectedyear, String roomname)
        {
            double[] res = new double[dayspermonth(selectedmonth,selectedyear)+1];
            bool notempty = false;
            using (ConnectionHandle conn = new ConnectionHandle(connectionpool,connectionstring,false))
            {
                String query = "select max(count) as mcount,xday from countstats where roomname=@roomname and xmonth=@xmonth and xyear=@xyear and cat=2 group by xday";
                try
                {
                    if (!connectioncheck(conn))
                        throw new Exception("Connection is not open");
                    using (var cmd = new NpgsqlCommand(query, conn.conn))
                    {
                        addParameters(cmd,
                            "roomname", roomname,
                            "xmonth", selectedmonth,
                            "xyear", selectedyear);
                        using (var reader = cmd.ExecuteReader())
                        {
                            while (reader.Read())
                            {
                                notempty = true;
                                res[(int)reader.GetDouble(1)] = reader.GetDouble(0);
                            }
                        }
                    }
                }
                catch (Exception ex)
                {
                    manageDbException(ex, conn.conn, true);
                    notempty = false;
                }
            }
            return notempty?res:null;
        }

        internal double[][] loadAvgDevicesTime(int selectedmonth, int selectedyear, string roomname)
        {
            int numdays = dayspermonth(selectedmonth, selectedyear);
            double[][] res = new double[numdays + 1][];
            double[] hourcount = new double[24];
            for(int i=0;i<numdays+1;i++)
                res[i]=new double[24];
            
            using (ConnectionHandle conn = new ConnectionHandle(connectionpool,connectionstring,false))
            {
                String query = "select avg(count) as mcount,xday,xhour from countstats where roomname=@roomname and xmonth=@xmonth and xyear=@xyear and cat=2 group by xhour, xday";
                try
                {
                    if (!connectioncheck(conn))
                        throw new Exception("Connection is not open");
                    using (var cmd = new NpgsqlCommand(query, conn.conn))
                    {
                        addParameters(cmd,
                            "roomname", roomname,
                            "xmonth", selectedmonth,
                            "xyear", selectedyear);
                        using (var reader = cmd.ExecuteReader())
                        {
                            while (reader.Read())
                            {
                                res[(int)reader.GetDouble(1)][(int)reader.GetDouble(2)] = reader.GetDouble(0);
                                hourcount[(int)reader.GetDouble(2)] += 1;
                            }
                        }
                    }
                }
                catch (Exception ex)
                {
                    manageDbException(ex, conn.conn, true);
                    return null;
                }
            }
            for(int hour=0;hour<24;hour++)
            {
                for (int day = 1; day < res.GetLength(0);day++)
                    res[0][hour] += res[day][hour];
                if(hourcount[hour]>0)
                    res[0][hour] /= hourcount[hour];
            }
            return res;
        }

        internal int[][,] loadHeathmaps(object p, string roomname, double xlength, double ylength, int selectedmonth, int selectedyear, double resolution, double filteruncertainity)
        {
            int numdays = dayspermonth(selectedmonth, selectedyear);
            int[][,] res = new int[numdays + 1][,];
            for(int j=0;j<numdays+1;j++)
                res[j]= new int[(int)(resolution*xlength)+1, (int)(resolution*ylength) + 1];

            using (ConnectionHandle conn = new ConnectionHandle(connectionpool,connectionstring,false))
            {
                String query = "select xpos,ypos,xday from devicespositions where roomname=@roomname and xmonth=@xmonth and xyear=@xyear and uncertainty<@uncertainty and outmovement=false";
                try
                {
                    if (!connectioncheck(conn))
                        throw new Exception("Connection is not open");
                    using (var cmd = new NpgsqlCommand(query, conn.conn))
                    {
                        addParameters(cmd,
                            "roomname", roomname,
                            "xmonth", selectedmonth,
                            "xyear", selectedyear,
                            "uncertainty", filteruncertainity);
                        using (var reader = cmd.ExecuteReader())
                        {
                            while (reader.Read())
                            {
                                int day = (int)(reader.GetDouble(2));
                                int xpos = (int)(resolution * reader.GetDouble(0));
                                int ypos = (int)(resolution * reader.GetDouble(1));
                                // numeric error? Rarely has happened this condition. Should have been solved, but let's be safe
                                if (day < res.Length && xpos < res[day].GetLength(0) && ypos < res[day].GetLength(1))
                                    res[day][xpos, ypos] += 1;
                            }
                        }
                    }
                }
                catch (Exception ex)
                {
                    manageDbException(ex, conn.conn, true);
                    return new int[numdays + 1][,];
                }
            }
            for (int xpos = 0; xpos < res[0].GetLength(0); xpos++)
            {
                for(int ypos=0;ypos<res[0].GetLength(1);ypos++)
                {
                    for (int day = 1; day < res.Length; day++)
                        res[0][xpos,ypos] += res[day][xpos,ypos];
                }               
            }
            return res;
        }

        internal string[,] loadFrequentMacs(int month, int year, string roomname)
        {
            int maxres = 15;
            int numdays = dayspermonth(month, year);
            String[,] res = new String[numdays + 1, maxres];
            String query = "(select identifier,count(*) as cnt,xday as dt from devicespositions where roomname=@roomname1 and xmonth=@xmonth1 and xyear=@xyear1 group by identifier, xday) union (select identifier,count(*) as cnt,0 as dt from devicespositions where roomname=@roomname2 and xmonth=@xmonth2 and xyear=@xyear2 group by identifier) order by dt,cnt desc";
            int preday = -1;
            int cntperday = 0;
            using (ConnectionHandle conn = new ConnectionHandle(connectionpool,connectionstring,false))
            {
                try
                {
                    if (!connectioncheck(conn))
                        throw new Exception("Connection is not open");
                    using (var cmd = new NpgsqlCommand(query, conn.conn))
                    {
                        addParameters(cmd,
                            "roomname1", roomname,
                            "xmonth1", month,
                            "xyear1", year,
                            "roomname2", roomname,
                            "xmonth2", month,
                            "xyear2", year);
                        using (var reader = cmd.ExecuteReader())
                        {
                            while (reader.Read())
                            {
                                int day = (int)reader.GetDouble(2);
                                String id = reader.GetString(0);
                                if (preday != day)
                                {
                                    cntperday = 0;
                                    preday = day;
                                }
                                if (cntperday < maxres)
                                {
                                    res[day, cntperday] = id;
                                    cntperday++;
                                }
                            }
                        }
                    }
                }
                catch(Exception ex)
                {
                    manageDbException(ex, conn.conn, true);
                    return new String[numdays + 1, maxres];
                }
            }
            return res;
        }

        internal class DeviceInfo
        {
            internal DateTime FirstSeen;
            internal DateTime LastSeen;
            internal String[] ssids;
        }

        internal DeviceInfo loadDeviceInfo(string id)
        {
            DeviceInfo di = new DeviceInfo();
            LinkedList<String> ssids = new LinkedList<string>();
            using (ConnectionHandle conn = new ConnectionHandle(connectionpool,connectionstring,false))
            {
                String query = "select min(tm),max(tm) from devicespositions where identifier=@identifier";
                String query2 = "select ssid from requestedssids where identifier=@identifier";
                try
                {
                    if (!connectioncheck(conn))
                        throw new Exception("Connection is not open");
                    using (var cmd = new NpgsqlCommand(query, conn.conn))
                    {
                        addParameters(cmd,
                            "identifier", id);
                        using (var reader = cmd.ExecuteReader())
                        {
                            if (reader.Read())
                            {
                                try
                                {
                                    di.FirstSeen = reader.GetDateTime(0);
                                    di.LastSeen = reader.GetDateTime(1);
                                }
                                catch (Exception)
                                {
                                    return null;
                                }
                            }
                            else
                            {
                                return null;
                            }
                        }
                    }
                    using (var cmd = new NpgsqlCommand(query2, conn.conn))
                    {
                        addParameters(cmd,
                            "identifier", id);
                        using (var reader = cmd.ExecuteReader())
                        {
                            while (reader.Read())
                            {
                                ssids.AddLast(reader.GetString(0));
                            }
                        }
                    }
                }
                catch(Exception ex)
                {
                    manageDbException(ex, conn.conn, true);
                    return null;
                }
            }
            di.ssids = ssids.ToArray<String>();
            return di;
        }

        internal class DeviceStats
        {
            internal Double[] timeperday;
            internal Double[] pingsperhour=new Double[24*6];
            internal int[,] heatmap;
            internal Dictionary<String, Int32> roommap = new Dictionary<string, int>();
        }

        internal DeviceStats loadDeviceStats(DateTime fromdate, String fromtime, DateTime todate, String totime, string deviceid, string roomname, bool loadroommap, bool loadheathmap, double xlen, double ylen, double htresolution, double filteruncertainity)
        {
            DeviceStats ds = new DeviceStats();
            if(loadheathmap)
                ds.heatmap =  new int[(int)(htresolution*xlen) + 1, (int)(htresolution*ylen) + 1];
            if (loadroommap)
                ds.roommap.Add("__OVERALL__", 0);
            ds.timeperday = new Double[(int)todate.Subtract(fromdate).TotalDays + 1];
            String query;
            if (loadroommap)
                query = "select xpos,ypos,tm,roomname,uncertainty from devicespositions where identifier=@identifier and tm>=@tmstart and tm<=@tmend and roomname not in (select roomname from rooms where archived=True) order by tm asc";
            else
                query = "select xpos,ypos,tm,roomname,uncertainty from devicespositions where identifier=@identifier and tm>=@tmstart and tm<=@tmend and roomname=@roomname and outmovement=false order by tm asc";
            DateTime pre = DateTime.MinValue;
            using (ConnectionHandle conn = new ConnectionHandle(connectionpool,connectionstring,false))
            {
                try
                {
                    if (!connectioncheck(conn))
                        throw new Exception("Connection is not open");
                    using (var cmd = new NpgsqlCommand(query, conn.conn))
                    {
                        if (!loadroommap)
                            cmd.Parameters.AddWithValue("roomname", roomname);
                        addParameters(cmd,
                            "identifier", deviceid,
                            "tmstart", new DateTime(fromdate.Year, fromdate.Month, fromdate.Day, Convert.ToInt32(fromtime.Split(':')[0]), Convert.ToInt32(fromtime.Split(':')[1]), 0),
                            "tmend", new DateTime(todate.Year, todate.Month, todate.Day, Convert.ToInt32(totime.Split(':')[0]), Convert.ToInt32(totime.Split(':')[1]), 0));
                        using (var reader = cmd.ExecuteReader())
                        {
                            while (reader.Read())
                            {
                                if (loadheathmap && reader.GetDouble(4) < filteruncertainity)
                                {
                                    int xpos = (int)(htresolution * reader.GetDouble(0));
                                    int ypos = (int)(htresolution * reader.GetDouble(1));
                                    if(xpos<ds.heatmap.GetLength(0)&&ypos<ds.heatmap.GetLength(1))
                                        ds.heatmap[xpos,ypos] += 1;
                                }
                                if (loadroommap)
                                {
                                    ds.roommap["__OVERALL__"] += 1;
                                    String room = reader.GetString(3);
                                    if (!ds.roommap.ContainsKey(room))
                                        ds.roommap.Add(room, 0);
                                    ds.roommap[room] += 1;
                                }
                                DateTime detectiontime = reader.GetDateTime(2);
                                if (detectiontime.Subtract(pre).TotalMinutes < 15)
                                    ds.timeperday[(int)detectiontime.Subtract(fromdate).TotalDays] += detectiontime.Subtract(pre).TotalMinutes;
                                else if (ds.timeperday[(int)detectiontime.Subtract(fromdate).TotalDays]==0)
                                    ds.timeperday[(int)detectiontime.Subtract(fromdate).TotalDays] = 0.001;
                                ds.pingsperhour[detectiontime.Hour * 6 + detectiontime.Minute / 10]++;
                                pre = detectiontime;
                            }
                        }
                    }
                }
                catch (Exception ex)
                {
                    pre = DateTime.MinValue;
                    manageDbException(ex, conn.conn, true);
                }
            }
            return pre!=DateTime.MinValue?ds:null;
        }
    }
}
