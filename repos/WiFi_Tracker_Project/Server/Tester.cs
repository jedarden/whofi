using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using System.Windows;

namespace Panopticon
{
    class Tester
    {
		public delegate void SafeCaller();
		public double dist2RSSI(double dist)
        {
            //DARIO: dubbio da merge
			// è giusta la versione commentata?
			
			/*if (dist < 1)
                return -25;
            return -30 * (Math.Log(dist, 5) + 1);*/
            return dist;
        }
        public double pos2dist(double x, double y, double sx, double sy)
        {
            System.Diagnostics.Debug.Print("Inserted with position " + x + " " + y);
            return Math.Sqrt((sx - x) * (sx - x) + (sy - y) * (sy - y));
        }
        Context ctx;
        Random rand;
        public Tester(Context _ctx)
        {
            ctx = _ctx;
            rand = new Random();
        }
        public void test()
        {
            Room r = ctx.getRoom("TestRoom");
            Station s1 = ctx.tryAddStation("0.0", null, false);
            Station s2 = ctx.tryAddStation("25.0", null, false);
            Station s3 = ctx.tryAddStation("0.25", null, false);
            Station s4 = ctx.createStation(r, "25.25", 25, 25, null);
            Station s5 = ctx.createStation(r, "12.12", 12, 12, null);
            Station s6 = ctx.createStation(r, "18.6", 18, 6, null);
            String[] macs = { "abc1", "abc2", "abc3", "abc4", "abc5" };
            String[] ssids = { "ssid1", "ssid2", "ssid3", "ssid4", "ssid5" };
            double toterror = 0;
            int nerrors = 0;
            while (true)
            {
                Packet p = new Packet(macs[rand.Next(0, macs.Length)], ssids[rand.Next(0, macs.Length)], DateTime.Now, "", "", 0);
                double x = 35 * rand.NextDouble();
                double y = 35 * rand.NextDouble();
                System.Diagnostics.Debug.Print("Inserted with real position " + x + " " + y);
                double xf=0, yf=0, xt=0, yt=0;
                xf = x + ((rand.NextDouble() * 2) - 1);
                yf = y + ((rand.NextDouble() * 2) - 1);
                xt += xf;
                yt += yf;
                p.received(s1, dist2RSSI(pos2dist(xf, yf, 0, 0)));
                xf = x + ((rand.NextDouble() * 2) - 1);
                yf = y + ((rand.NextDouble() * 2) - 1);
                xt += xf;
                yt += yf;
                p.received(s2, dist2RSSI(pos2dist(xf, yf, 25, 0)));
                xf = x + ((rand.NextDouble() * 2) - 1);
                yf = y + ((rand.NextDouble() * 2) - 1);
                xt += xf;
                yt += yf;
                p.received(s3, dist2RSSI(pos2dist(xf, yf, 0, 25)));
                /*xf = x + ((rand.NextDouble() * 2) - 1);
                yf = y + ((rand.NextDouble() * 2) - 1);
                xt += xf;
                yt += yf;
                p.received(s4, dist2RSSI(pos2dist(xf, yf, 25, 25)));
                xf = x + ((rand.NextDouble() * 2) - 1);
                yf = y + ((rand.NextDouble() * 2) - 1);
                xt += xf;
                yt += yf;
                p.received(s5, dist2RSSI(pos2dist(xf, yf, 12, 12)));
                xf = x + ((rand.NextDouble() * 2) - 1);
                yf = y + ((rand.NextDouble() * 2) - 1);
                xt += xf;
                yt += yf;
                p.received(s6, dist2RSSI(pos2dist(xf, yf, 18, 6)));*/
                //p.testposition = new PositionTools.Position(x, y, r);
                xt /= p.Receivings.Count;
                yt /= p.Receivings.Count;
                System.Diagnostics.Debug.Print("Averaged: ", xt, " ", yt);
                double error = Math.Sqrt(Math.Pow((xt - x), 2) + Math.Pow((yt - y), 2));
                toterror += error;
                nerrors++;
                System.Diagnostics.Debug.Print("Averaged Error: " + error);
                System.Diagnostics.Debug.Print("Averaged Mean Error: " + toterror/nerrors);
                ctx.getAnalyzer().sendToAnalysisQueue(p);
                Thread.Sleep((int)(rand.NextDouble() * 1000));
            }
        }

		public void testFEDE()
		{
			Console.WriteLine("Programma avviato");

		}

	}
}
