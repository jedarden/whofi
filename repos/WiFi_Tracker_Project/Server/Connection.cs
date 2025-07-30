using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Net.Sockets;
using System.Net;
using System.Windows;

namespace Panopticon
{
	class Connection
	{
		private readonly Socket serverSocket; //serverSocket is used only to accept new boards asking for registration
		protected readonly List<Socket> clientSockets;
		private const int BUFFER_SIZE = 2048;
		private const int PORT = 1500;
		private readonly byte[] buffer;
		private Context ctx;
		private Protocol protocol;

		public Connection(Context _ctx)
		{
			serverSocket = new Socket(AddressFamily.InterNetwork, SocketType.Stream, ProtocolType.Tcp);
			clientSockets = new List<Socket>();
			buffer = new byte[BUFFER_SIZE];
			ctx = _ctx;
			protocol = new Protocol(ctx);
		}

        /// <summary>
        /// This method initializes a passive listening socket for communications over internet
        /// </summary>
        public void StartConnection()
        {
            Console.WriteLine("Setting up server...");
			try
			{
				serverSocket.Bind(new IPEndPoint(IPAddress.Any, PORT));
				serverSocket.Listen(0);
				serverSocket.BeginAccept(AcceptCallback, null);
			}
			catch (SocketException)
			{
				serverSocket.Close();
				System.Console.WriteLine("Impossible to open server socket");
				ctx.kill();
				return;
			}
			catch (ObjectDisposedException)
			{
				serverSocket.Close();
				System.Console.WriteLine("Server socket not available");
				ctx.kill();
				return;
			}
			Console.WriteLine("Server setup complete");
        }

        /// <summary>
        /// Close all connected client (we do not need to shutdown the server socket as its connections
        /// are already closed with the clients).
        /// </summary>
        private void CloseAllSockets()
        {
            foreach (Socket socket in clientSockets)
            {
                socket.Shutdown(SocketShutdown.Both);
                socket.Close();
            }
            clientSockets.Clear();
            serverSocket.Close();
        }

        private void AcceptCallback(IAsyncResult AR)
        {
            Socket socket;

            try
            {
                socket = serverSocket.EndAccept(AR); //the socket obtained will be always the same for connections with the requesting board
                socket.SetSocketOption(SocketOptionLevel.Socket, SocketOptionName.ReuseAddress, 1);
            }
            catch (ObjectDisposedException) // I cannot seem to avoid this (on exit when properly closing sockets)
            {
				return;
            }

            clientSockets.Add(socket);
			try
			{
				socket.BeginReceive(buffer, 0, BUFFER_SIZE, SocketFlags.None, ReceiveCallback, socket);
			}
			catch (Exception)
			{
				socket.Close();
				clientSockets.Remove(socket);
			}
			Console.WriteLine("Client connected, waiting for request...");
			try
			{
				serverSocket.BeginAccept(AcceptCallback, null);
			}
			catch (Exception)
			{
				serverSocket.Close();
				MessageBox.Show("Sorry, a fatal error on server socket has occurred. Restart Panopticon to pair new stations", "Network error");
			}
        }

        //when an ESP board sends a command, this Callback manages it
        private void ReceiveCallback(IAsyncResult AR)
        {
            Console.WriteLine("\n--Entering in a Receive callback--");
            Socket current = (Socket)AR.AsyncState;
            int received;

            try
            {
                received = current.EndReceive(AR); //it returns num of bytes received
            }
            catch (Exception)
            {
                Console.WriteLine("Client forcefully disconnected");
                current.Close();
                clientSockets.Remove(current);
                return;
            }
			
            byte[] recBuf = new byte[received];
            Array.Copy(buffer, recBuf, received);
            string text = Encoding.UTF8.GetString(recBuf);
            Console.WriteLine("Received Text: " + text);

            int result = protocol.Command(text, current, received);
			try
			{
				current.BeginReceive(buffer, 0, BUFFER_SIZE, SocketFlags.None, ReceiveCallback, current);
			}
			catch (Exception)
			{
				current.Close();
			}
        }

		internal void kill()
		{
			serverSocket.Close();
			protocol.kill();
		}
    }
}
