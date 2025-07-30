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
    /// Logica di interazione per NewRoom.xaml
    /// </summary>
    public partial class NewRoom : Page
    {
		Context ctx;
		StationHandler handler;
		static float maxMeters = 40;
        public NewRoom()
        {
            InitializeComponent();
        }

		internal NewRoom(Context _ctx, StationHandler _handler)
		{
			InitializeComponent();
			ctx = _ctx;
			handler = _handler;
		}

		private void Button_Cancel(object sender, RoutedEventArgs e)
		{
			this.NavigationService.GoBack();
		}

		private void Button_Continue(object sender, RoutedEventArgs e)
		{
			//because '.' isn't recognized as floating point
			TB_RoomWidth.Text = TB_RoomWidth.Text.Replace('.', ',');
			TB_RoomHeight.Text = TB_RoomHeight.Text.Replace('.', ',');

			//firstly, I check that all form fields are correctly filled
			int error = 0;
			if (TB_RoomName.Text == "Room Name" || ctx.checkRoomExistence(TB_RoomName.Text) == true)
			{
				TB_RoomName.Foreground = Brushes.Red;
				error++;
			}
			if (TB_RoomWidth.Text == "Room Width" || !StringFloatMajorThan0(TB_RoomWidth.Text))
			{
				TB_RoomWidth.Foreground = Brushes.Red;
				error++;
			}
			if (TB_RoomHeight.Text == "Room Height" || !StringFloatMajorThan0(TB_RoomHeight.Text))
			{
				TB_RoomHeight.Foreground = Brushes.Red;
				error++;
			}
			if (error > 0)
				return;
			else //if there aren't errors, I save the room
			{
				Room r = new Room(TB_RoomName.Text, float.Parse(TB_RoomWidth.Text), float.Parse(TB_RoomHeight.Text));
				ctx.createRoom(r);
                if (!ctx.saveRoom(r))
                    MessageBox.Show("Error while saving the room on persistent storage. The room will be working for the present session, but will have to be created again at reboot.");
				this.NavigationService.Navigate(new AddToRoom(ctx, r, handler));
			}
		}

		//methods dedicated to check correct form filling
		private void TextBoxName_LostFocus(object sender, RoutedEventArgs e)
		{
			string defaultText = "Room Name";
			if (TB_RoomName.Text == "")
			{
				TB_RoomName.Foreground = Brushes.Gray;
				TB_RoomName.Text = defaultText;
			}
			else if (TB_RoomName.Text == defaultText)
				TB_RoomName.Foreground = Brushes.Gray;
			else if (TB_RoomName.Text != defaultText)
				TB_RoomName.Foreground = Brushes.Black;
		}
		private void TextBoxWidth_LostFocus(object sender, RoutedEventArgs e)
		{
			string defaultText = "Room Width";
			if (TB_RoomWidth.Text == "")
			{
				TB_RoomWidth.Foreground = Brushes.Gray;
				TB_RoomWidth.Text = defaultText;
			}
			else if (TB_RoomWidth.Text == defaultText)
				TB_RoomWidth.Foreground = Brushes.Gray;
			else if (TB_RoomWidth.Text != defaultText)
				TB_RoomWidth.Foreground = Brushes.Black;
		}
		private void TextBoxHeight_LostFocus(object sender, RoutedEventArgs e)
		{
			string defaultText = "Room Height";
			if (TB_RoomHeight.Text == "")
			{
				TB_RoomHeight.Foreground = Brushes.Gray;
				TB_RoomHeight.Text = defaultText;
			}
			else if (TB_RoomHeight.Text == defaultText)
				TB_RoomHeight.Foreground = Brushes.Gray;
			else if (TB_RoomHeight.Text != defaultText)
				TB_RoomHeight.Foreground = Brushes.Black;
		}
		private bool StringFloatMajorThan0(string _text)
		{
			float f;
			if (!float.TryParse(_text, out f))
				return false;
			else if (f < 0)
			{
				MessageBox.Show("Room dimension can't be negative value", "Error");
				return false;
			}
			else if (f > maxMeters)
			{
				MessageBox.Show(String.Format("Room dimension must be lower than {0} m", maxMeters), "Error");
				return false;
			}
			return true;
		}
	}
}
