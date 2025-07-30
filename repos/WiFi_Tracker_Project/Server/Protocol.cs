using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Net;
using System.Net.Sockets;
using System.Text;
using System.Threading.Tasks;
using System.Threading;
using System.IO;
using System.Windows;

namespace Panopticon
{
    class Protocol
    {
		public delegate void SafeNewStation(string _string, Socket socket);
		private const int BUFFER_SIZE = 2048;
		internal Dictionary<Socket, string> macAddresses;
		Context ctx;
		FileParser fileParser;
		
		public Protocol(Context _ctx)
		{
			macAddresses = new Dictionary<Socket, string>();
			ctx = _ctx;
			fileParser = new FileParser(ctx);
		}
		
		/// <summary>
        /// Interprets the command received through the socket           
        /// </summary>
        public int Command(string text, Socket socket, int received)
        {
            int x;
            int toReceive = 0;
            int fileSize = 0;
            int timestamp = 0;
			DateTime timestampDT = new DateTime();
			bool ignoring = false; //true if there's any reason to ignore the file received

			if (text.IndexOf("REGISTER(") > -1) //-----------------------REGISTER------------------------
			{
				int offset = text.IndexOf("REGISTER(");
				string macAddress = text.Substring(offset + 9, 17);
				macAddresses.Add(socket, macAddress.Replace(@":", string.Empty));
				Console.WriteLine("An ESP board with MAC: " + macAddress + " has requested access");

				//apro finestra configurazione nuova scheda
				var d = new SafeNewStation(ctx.guiPub.linkedwindow.NewStation);
				Application.Current.Dispatcher.Invoke(d, macAddress.Replace(@":", string.Empty), socket);

				byte[] data = Encoding.UTF8.GetBytes("ACCEPT\r\n");
				try
				{
					socket.Send(data); //blocking method
				}
				catch (Exception)
				{
					macAddresses.Remove(socket);
					socket.Close();
					return -1;
				}
				int result = ESP_SyncClock(socket);
				if (result == -1) return -1;
			}
			else if ((x = text.IndexOf("FILE")) > -1) //-------------------FILE----------------------------
			{
				//FILE\r\n BBBB TTTT 010101010101010101010...

				byte[] buffer = Encoding.UTF8.GetBytes(text);
				byte[] data = new byte[BUFFER_SIZE];
				//copio l'array "data" nel buffer di ricezione
				for (int i = 0; i < received; i++)
				{
					data[i] = buffer[i];
				}
				try
				{
					while (received < 14)
					{
						//ricevo di nuovo, finché non ricevo tutti i metadati
						int receivedBytesLen = socket.Receive(data, received, BUFFER_SIZE - received, SocketFlags.None);
						received += receivedBytesLen;
					}

					//ora ho ricevuto i metadati
					if (received > 14)
					{
						//leggere quanti byte si devono ricevere e timestamp
						byte[] fileSizeBytes = new byte[4];
						byte[] timestampBytes = new byte[4];
						byte[] timestampBytesReversed = new byte[4];

						for (int i = 0; i < 4; i++)
						{
							fileSizeBytes[3 - i] = data[i + 6];
						}
						for (int i = 0; i < 4; i++)
						{
							timestampBytes[3 - i] = data[i + 10];
						}

						fileSize = BitConverter.ToInt32(fileSizeBytes, 0);
						timestamp = BitConverter.ToInt32(timestampBytes, 0);
						timestampDT = FileParser.TimeFromUnixTimestamp(timestamp);
						toReceive = fileSize - received + 14; //bytes che devo ancora ricevere
					}

					string toAnalyze = Encoding.UTF8.GetString(data, 14, data.Length - 14);
					string chunk = Chunker(toAnalyze, out toAnalyze);
					Station station = ctx.getStation(macAddresses[socket]); //dal socket trovo la station
					if (station == null) //probably, the station isn't configured yet
					{
						ignoring = true; //received file will be ignored
					}
					else
					{
						station.hearbeat();
					}

					if (!ignoring) fileParser.ParseOnTheFly(chunk, station);

					while (toReceive > 0)
					{
						Array.Clear(data, 0, BUFFER_SIZE); //svuoto buffer ricezione
						var ttt = Encoding.UTF8.GetBytes(toAnalyze);
						ttt.CopyTo(data, 0); //ripristino data con il residuo della vecchia ricezione
						//ricevo ancora
						int receivedBytesLen = socket.Receive(data, toAnalyze.Length, BUFFER_SIZE - toAnalyze.Length, SocketFlags.None);
						received += receivedBytesLen;
						toReceive = toReceive - receivedBytesLen;
						toAnalyze = Encoding.UTF8.GetString(data);
						chunk = Chunker(toAnalyze, out toAnalyze);
						if (!ignoring) fileParser.ParseOnTheFly(chunk, station);
					}
				}
				catch (Exception)
				{
					macAddresses.Remove(socket);
					socket.Close();
					return -1;
				}

				Console.WriteLine("{1} Client:{0} data received.", socket.RemoteEndPoint, DateTime.Now.ToString());

			}
			else if (text.IndexOf("SYNC") > -1) //----------------------SYNC----------------------------
			{
				int result = ESP_SyncClock(socket);
				if (result == -1) return -1;
			}
			else return -1;

            return 0; //if here, all fine
        }

		/// <summary>
		/// This method returns a chunk of raw receptions, ready to be parsed into Reception objects.
		/// </summary>
		/// <param name="toAnalyze">input data</param>
		/// <param name="toAnalyze2">input data - chunk</param>
		/// <returns>chunk of data</returns>
		private string Chunker(string toAnalyze, out string toAnalyze2)
		{
			int index = toAnalyze.LastIndexOf("\n");
			if (index > 0)
			{
				toAnalyze2 = toAnalyze.Substring(index+1);
				return toAnalyze.Substring(0, index);
			}
			else
			{
				toAnalyze2 = "";
				return toAnalyze;
			}
		}

		/// <summary>
		/// Syncronizes clock of ESP board (Attention: it doesn't include SYNC message).
		/// After this phase, ESP board will begin the sniffing.
		/// </summary>
		public static int ESP_SyncClock(Socket socket)
        {
			try
			{
				Console.WriteLine("Starting CLOCK sync");
				int n = 5; //number of PING-PONG repetitions
				int received, i;
				byte[] ping = Encoding.UTF8.GetBytes("PING\r\n");
				byte[] clock1 = Encoding.UTF8.GetBytes("CLOCK(");
				byte[] clock2 = Encoding.UTF8.GetBytes(")\r\n");
				byte[] recBuf = new byte[100];
				byte[] buffer = new byte[100];

				long frequency = Stopwatch.Frequency;
				Stopwatch sw = new Stopwatch();
				sw.Start(); //--------------------------------------START_TIMER
				for (i = 0; i < n; i++)
				{
					socket.Send(ping); //blocking method
					Console.WriteLine("PING sent");

					received = socket.Receive(recBuf); //blocking method
					byte[] recMsg = new byte[received];
					Array.Copy(recBuf, recMsg, received);
					string text = Encoding.UTF8.GetString(recMsg);

					if (received == 0) return -1;
					if (text.IndexOf("PONG") > -1)
					{
						Console.WriteLine("Received PONG");
						continue;
					}
					else break;
				}
				sw.Stop(); //--------------------------------------STOP_TIMER
				if (i < 5) return -1;
				long ticksElapsed = sw.ElapsedTicks * 10000000 / frequency;
				ticksElapsed = ticksElapsed / n / 2;
				DateTime ESP_Time = new DateTime(DateTime.Now.Ticks + ticksElapsed);
				//UE will be Unix Epoch version of the time
				TimeSpan clockUE = ESP_Time - new DateTime(1970, 1, 1);
				uint timestamp = (uint)clockUE.TotalSeconds; //secondi totali dall'anno 0
				byte[] toSend = Encoding.UTF8.GetBytes("CLOCK(" + timestamp.ToString() + ")\r\n");
				if (socket.Send(toSend) > 0)
					Console.WriteLine("Clock {0} sent to ESP board", timestamp.ToString());

				return 0;
			}
			catch (Exception)
			{
				socket.Close();
				return -1;
			}
		}

        /// <summary>
        /// Starts blinking to localize ESP board
        /// </summary>
        public static void ESP_BlinkStart(Socket socket)
        {
            byte[] data = Encoding.UTF8.GetBytes("BLINK\r\n");
			try
			{
				socket.Send(data); //blocking method
			}
			catch (Exception)
			{
				socket.Close();
				return;
			}
			Console.Write("Blinking... ");
		}

        /// <summary>
        /// Syncronizes clock of ESP board in any arbitrary time (SYNC version)
        /// </summary>
        public static void ESP_SyncClockRequest(Socket socket)
        {
            byte[] data = Encoding.UTF8.GetBytes("SYNC\r\n");
			try
			{
				socket.Send(data); //blocking method
			}
			catch (Exception)
			{
				socket.Close();
				return;
			}
        }

        /// <summary>
        /// Wakes ESP board from stand-by state and starts sniffing again
        /// </summary>
        public static void ESP_BlinkStop(Socket socket)
        {
            byte[] data = Encoding.UTF8.GetBytes("OKLED\r\n");
			try
			{
				socket.Send(data); //blocking method
			}
			catch (Exception)
			{
				socket.Close();
				return;
			}
			Console.WriteLine("Blinking stopped");
		}

        /// <summary>
        /// Puts ESP board in a stand-by state, i.e. stops sniffing
        /// </summary>
        public static void ESP_StandBy(Socket socket)
        {
            byte[] data = Encoding.UTF8.GetBytes("STANDBY\r\n");
			try
			{
				socket.Send(data); //blocking method
			}
			catch (Exception)
			{
				socket.Close();
				return;
			}
		}

        /// <summary>
        /// Syncronize clock of ESP board
        /// </summary>
        public static void ESP_Resume(Socket socket)
        {
            byte[] data = Encoding.UTF8.GetBytes("RESUME\r\n");
			try
			{
				socket.Send(data); //blocking method
			}
			catch (Exception)
			{
				socket.Close();
				return;
			}
		}

        /// <summary>
        /// Reboots ESP board
        /// </summary>
        public static void ESP_Reboot(Socket socket)
        {
			System.Console.WriteLine("Rebooting...");
            byte[] data = Encoding.UTF8.GetBytes("REBOOT\r\n");
			int i = 0;
			try
			{
				while (i<10*60*5)
				{
					socket.Send(data); //blocking method
                    Thread.Sleep(101);
					i++;
				}
			}
			catch (Exception)
			{
				System.Console.WriteLine("Said {0} times", i);
				socket.Close(); //to cancel all callbacks registered on old socket
				return;
			}
        }

		internal void kill()
		{
			fileParser.kill();
		}
	}
}
