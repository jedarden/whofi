using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;

namespace Panopticon.AddingStationPages
{
    /// <summary>
    /// Logica di interazione per AddToRoom.xaml
    /// </summary>
    public partial class AddToRoom : Page
    {
		private Room room;
		Context ctx;
		double meterX=0, meterY=0;
		StationHandler handler = null;
		static int maxN = 40;
		int radius = 1;

		public AddToRoom()
        {
            InitializeComponent();
        }

		internal AddToRoom(Context _ctx, Room _room, StationHandler _handler)
		{
			InitializeComponent();
			room = _room;
			ctx = _ctx;
			handler = _handler;
			Label_RoomName.Content = room.roomName;
			handler.switchLedBlink(true);

			//dimensiono graficamente la stanza
			if (GridRoom.Width > 100 || GridRoom.Height > 100)
			{
				GridRoom.Width = room.size.X;
				GridRoom.Height = room.size.Y;
				Canvas_Room.Width = room.size.X;
				Canvas_Room.Height = room.size.Y;
			}
			else
			{
				GridRoom.Width = room.size.X * 100;
				GridRoom.Height = room.size.Y * 100;
				Canvas_Room.Width = room.size.X * 100;
				Canvas_Room.Height = room.size.Y * 100;
			}

			ComputeRadius(); //calcolo raggio grafico
			LoadStations();
		}

		void LoadStations()
		{
			foreach (Station s in room.getStations())
			{
				//disegno stazione
				Ellipse e = new Ellipse() { Width = 2 * radius, Height = 2 * radius, Fill = Brushes.Blue };
				Canvas_Room.Children.Add(e);
				r2g(s.location.X, 0);
				Canvas.SetLeft(e, r2g(s.location.X, 0) - radius);
				Canvas.SetTop(e, r2g(s.location.Y, 1) - radius);
			}
		}

		private void Grid_MouseMove(object sender, MouseEventArgs e)
		{
			//room.xlenght è la dimensione in cm della stanza
			//GridRoom.Width è la dimensione in dpi della stanza

			double meterX = Mouse.GetPosition(this.GridRoom).X / this.GridRoom.Width * room.size.X;
			double meterY = Mouse.GetPosition(this.GridRoom).Y / this.GridRoom.Height * room.size.Y;
			if (meterX < 0) meterX = 0;
			if (meterY < 0) meterY = 0;
			if (meterX > room.size.X)
                meterX = room.size.X;
			if (meterY > room.size.Y)
                meterY = room.size.Y;
			xLabel.Content = meterX.ToString("X: 0.## m");
			yLabel.Content = meterY.ToString("Y: 0.## m");

		}

		/// <summary>
		/// It returns a measure converted in a graphic measure (ready to be used as graphic coordinate)
		/// The 2nd argument will be 0 for X, 1 for Y
		/// </summary>
		double r2g(double realMeasure, int XorY) //real to graphic (measure)
		{
			if (XorY == 0)
			{
				return realMeasure / room.size.X * this.GridRoom.Width;
			}
			else if (XorY == 1)
			{
				return realMeasure / room.size.Y * this.GridRoom.Height;
			}
			else throw new Exception();
		}

		private void Grid_NewPosition(object sender, MouseButtonEventArgs e)
		{
			meterX = Mouse.GetPosition(this.GridRoom).X / this.GridRoom.Width * room.size.X;
			meterY = Mouse.GetPosition(this.GridRoom).Y / this.GridRoom.Height * room.size.Y;
			if (meterX < 0) meterX = 0;
			if (meterY < 0) meterY = 0;
			if (meterX > room.size.X)
                meterX = room.size.X;
			if (meterY > room.size.Y)
                meterY = room.size.Y;
			LastX.Content = meterX.ToString("Last X: 0.## m");
			LastY.Content = meterY.ToString("Last Y: 0.## m");
			Button_ConfirmPosition.IsEnabled = true;
			Ellipse_StationPosition.Width = 2 * radius + 1;
			Ellipse_StationPosition.Height = 2 * radius + 1;
			Ellipse_StationPosition.SetValue(Canvas.LeftProperty, Mouse.GetPosition(this.GridRoom).X-radius);
			Ellipse_StationPosition.SetValue(Canvas.TopProperty, Mouse.GetPosition(this.GridRoom).Y-radius);
			if (!Ellipse_StationPosition.IsVisible)
				Ellipse_StationPosition.Visibility = Visibility.Visible;
		}

		void ComputeRadius()
		{
			double max = Math.Max(GridRoom.Height, GridRoom.Width);
			max /= 50;
			radius = Convert.ToInt32(max);
		}

		private void Button_Cancel(object sender, RoutedEventArgs e)
		{
			if (handler.isBlinking) handler.switchLedBlink(false);
			this.NavigationService.GoBack();
		}

		private void Button_ConfirmNewStation(object sender, RoutedEventArgs e)
		{
			//tryAddStation è già stato chiamato (ha lanciato la GUI)
			if (handler.isBlinking) handler.switchLedBlink(false);
			Station s = ctx.createStation(room, handler.macAddress, meterX, meterY, handler);
            if (!ctx.saveStation(s)) //salvataggio su DB
                MessageBox.Show("Error while saving the station association on persistant storage. The station is associated for the current session, but at reboot will have to be reconfigured");
			int i = FindWindowByMAC(handler.macAddress, maxN);
			if (i != maxN)
			{
				Application.Current.Windows[i].Close();
			}
		}

		internal int FindWindowByMAC(string _macAddress, int _maxN)
		{
			int i = 0;
			foreach (Window w in Application.Current.Windows)
			{
				if (i < _maxN)
					if (String.Compare((string)(w.Tag), _macAddress) == 0)
					{
						return i;
					}
					else i++;
				else
					break;
			}
			return _maxN;
		}
	}
}
