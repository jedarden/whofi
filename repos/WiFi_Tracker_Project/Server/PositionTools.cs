using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace Panopticon
{
    class PositionTools
    {
        private const double EXTERNAL_MARGIN = 3;
        internal const double UNCERTAIN_POSITION = 1000;
        public class Vector2D
        {
            public static Vector2D Zero = new Vector2D(0, 0);
            public double X;
            public double Y;
            public Vector2D(double x, double y)
            {
                X = x;
                Y = y;
            }
            public Vector2D(Vector2D a)
            {
                X = a.X;
                Y = a.Y;
            }
            public Vector2D Add(Vector2D b)
            {
                return new Vector2D(X + b.X, Y + b.Y);
            }

            public Vector2D _Add(Vector2D b)
            {
                X += b.X;
                Y += b.Y;
                return this;
            }

            public Vector2D AddScalar(double s)
            {
                return new Vector2D(X + s, Y + s);
            }
            public Vector2D _AddScalar(double s)
            {
                X += s;
                Y += s;
                return this;
            }
            public Vector2D Subtract(Vector2D b)
            {
                return new Vector2D(X - b.X, Y - b.Y);
            }

            public Vector2D _Subtract(Vector2D b)
            {
                X -= b.X;
                Y -= b.Y;
                return this;
            }

            public double Module()
            {
                return Math.Sqrt(X * X + Y * Y);
            }

            public double Dot(Vector2D b)
            {
                return X * b.X + Y * b.Y;
            }
            public Vector2D Multiply(Vector2D b)
            {
                return new Vector2D(X * b.X, Y * b.Y);
            }
            public Vector2D _Multiply(Vector2D b)
            {
                X *= b.X;
                Y *= b.Y;
                return this;
            }
            public Vector2D MultiplyScalar(double scalar)
            {
                return new Vector2D(X * scalar, Y * scalar);
            }

            public Vector2D _MultiplyScalar(double scalar)
            {
                X *= scalar;
                Y *= scalar;
                return this;
            }
            public Vector2D Divide(Vector2D b)
            {
                return new Vector2D(X / b.X, Y / b.Y);
            }
            public Vector2D _Divide(Vector2D b)
            {
                X /= b.X;
                Y /= b.Y;
                return this;
            }
            public Vector2D DivideScalar(double scalar)
            {
                return new Vector2D(X / scalar, Y / scalar);
            }
            public Vector2D _DivideScalar(double scalar)
            {
                X /= scalar;
                Y /= scalar;
                return this;
            }
            public Vector2D Clip(Vector2D min, Vector2D max)
            {
                double nx = X < min.X ? min.X : X;
                nx = nx > max.X ? max.X : nx;
                double ny = Y < min.Y ? min.Y : Y;
                ny = ny > max.Y ? max.Y : ny;
                return new Vector2D(nx, ny);
            }
            public Vector2D _Clip(Vector2D min, Vector2D max)
            {
                double nx = X < min.X ? min.X : X;
                nx = nx > max.X ? max.X : nx;
                double ny = Y < min.Y ? min.Y : Y;
                ny = ny > max.Y ? max.Y : ny;
                X = nx;
                Y = ny;
                return this;
            }

            public Vector2D Normalize(bool redirection)
            {
                double mod = this.Module();
                if (mod == 0)
                    return new Vector2D(0, 0);
                if (X < 0 && redirection)
                    mod *= -1;
                return new Vector2D(X / mod, Y / mod);
            }

            public Vector2D _Normalize(bool redirection)
            {
                double mod = this.Module();
                if (mod == 0)
                    return new Vector2D(0, 0);
                if (X < 0 && redirection)
                    mod *= -1;
                X /= mod;
                Y /= mod;
                return this;
            }

            public void _Import(Vector2D p)
            {
                this.X = p.X;
                this.Y = p.Y;
            }
        }

        public class Position : Vector2D
        {
            internal double uncertainity;
            internal Room room;
            internal DateTime positionDate;
            internal String tag;
            public Position(double x, double y, Room r) : base(x, y)
            {
                room = r;
            }
            public Position(double x, double y, Room r,Double u) : base(x, y)
            {
                room = r;
                uncertainity=u;
            }

            public Position(Vector2D a, Room r) : base(a)
            {
                room = r;
            }

            public Position(Vector2D a,Room r, Double u):base(a)
            {
                room = r;
                uncertainity = u;
            }

            public Position(Position a) : base(a)
            {
                room = a.room;
                positionDate = a.positionDate;
                uncertainity = a.uncertainity;
            }
        }
        class Circle : Vector2D
        {
            public double R;

            public Circle(double x, double y, double r) : base(x, y)
            {
                R = r;
            }
            public Circle(Vector2D a, double r) : base(a)
            {
                R = r;
            }
        }
        /// <summary>
        /// Compute the position given the receivings
        /// </summary>
        internal static Position triangulate(List<Packet.Reception> receivings, DateTime receptiontime)
        {
            Position p = new Position(Vector2D.Zero, receivings[0].ReceivingStation.location.room);
            Vector2D accumulator = new Vector2D(Vector2D.Zero);
            p.positionDate = receptiontime;
            String tag = ""; 
            if (receivings.Count() > 2)
            {
                double minrssi = 0;
                double avgrssi = 0;
                double vote = 0;
                foreach(Packet.Reception pr in receivings)
                {
                    tag += (Environment.NewLine + pr.RSSI + " " + pr.ReceivingStation.NameMAC);
                    minrssi = pr.RSSI < minrssi ? pr.RSSI : minrssi;
                    avgrssi += pr.RSSI;
                    double weight = Math.Pow(Math.E,(pr.RSSI + 25)/10);
                    accumulator = accumulator.Add(pr.ReceivingStation.location.MultiplyScalar(weight));
                    vote += weight;
                }
                avgrssi /= receivings.Count();
                accumulator = accumulator.DivideScalar(vote).Clip(Vector2D.Zero, p.room.size);
                tag += Environment.NewLine + "avg: " + accumulator.X + " " + accumulator.Y;
                if (avgrssi < -105 || (avgrssi < -90 && minrssi < -70))
                    p = new Position(0, 0, Room.externRoom);
                else
                {
                    if (receivings.Count() == 3)
                    {
                        Vector2D tr = triangulate_circles(receivings);
                        if (tr != null)
                        {
                            //tag += Environment.NewLine + "tri: " + tr.X + " " + tr.Y;
                            accumulator = accumulator.MultiplyScalar(0.87).Add(tr.Clip(Vector2D.Zero, p.room.size).MultiplyScalar(0.13));
                        }
                    }
                    p._Import(accumulator);
                }
            }
            else
                p.uncertainity = UNCERTAIN_POSITION + 1;
            p.tag = tag;
            return p;
        }
        /// <summary>
        /// Convert the rssi to metric distance
        /// </summary>
        private static double rssi2dis(double rssi)
        {
            rssi = -rssi - 30;
            rssi = rssi > 0 ? rssi : 0;
            double dist = Math.Pow(2, rssi / 8) - 1;
            return dist;
        }
        /// <summary>
        /// Find the instersection between three circumferences
        /// </summary>
        /// <param name="receivings"></param>
        /// <returns></returns>
        private static Vector2D triangulate_circles(List<Packet.Reception> receivings)
        {
            Circle a = new Circle(receivings[0].ReceivingStation.location, rssi2dis(receivings[0].RSSI));
            Circle b = new Circle(receivings[1].ReceivingStation.location, rssi2dis(receivings[1].RSSI));
            Circle c = new Circle(receivings[2].ReceivingStation.location, rssi2dis(receivings[2].RSSI));
            Vector2D col1 = a.Subtract(b).Normalize(true);
            Vector2D col2 = b.Subtract(c).Normalize(true);
            if (col1.Subtract(col2).Module() < 0.1)
            {
                return null;
            }
            Vector2D ex = b.Subtract(a).Normalize(false);
            double i = ex.Dot(c.Subtract(a));
            Vector2D ey = (c.Subtract(a).Subtract(ex.MultiplyScalar(i))).Normalize(false);
            double d = b.Subtract(a).Module();
            double j = ey.Dot(c.Subtract(a));
            double x = (a.R * a.R - b.R * b.R + d * d) / (2 * d);
            double y = (a.R * a.R - c.R * c.R + i * i + j * j) / (2 * j) - i * x / j;
            return new Vector2D(a.Add(ex.MultiplyScalar(x)).Add(ey.MultiplyScalar(y)));
        }
    }
}
