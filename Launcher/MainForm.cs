using System;
using System.ComponentModel;
using System.Diagnostics;
using System.Drawing;
using System.Net;
using System.Net.Sockets;
using System.Windows.Forms;

namespace Launcher {
	
	public partial class MainForm : Form {
		
		public const string AppName = "ClassicalSharp Launcher 0.9";
		
		public MainForm() {
			InitializeComponent();
			AdjustTabs();
			SetTooltips();
			// hide tabs at start
			tabMC.TabPages.Remove( tabMCServers );
			tabCC.TabPages.Remove( tabCCServers );
			Shown += DisplayResourcesDialog;
			
			mc = new GameState() { Progress = prgMCStatus, Status = lblMCStatus,
				Username = txtMCUser, Password = txtMCPassword, HostServer = "minecraft.net",
				Session = new MinecraftSession(), SignInButton = btnMCSignIn, Tab = tabMC,
				ServersTab = tabMCServers, ServersTable = tblMCServers, Hash = txtMCHash,
				form = this };
			cc = new GameState() { Progress = prgCCStatus, Status = lblCCStatus,
				Username = txtCCUser, Password = txtCCPassword, HostServer = "classicube.net",
				Session = new ClassicubeSession(), SignInButton = btnCCSignIn, Tab = tabCC,
				ServersTab = tabCCServers, ServersTable = tblCCServers, Hash = txtCCHash,
				form = this };
			
			mc.Filter = e =>
				// NOTE: using ToLower().Contains() allocates too many unecessary strings.
				e.Name.IndexOf( txtMCSearch.Text, StringComparison.OrdinalIgnoreCase ) >= 0
				&& ( cbMCHideEmpty.Checked ? e.Players[0] != '0' : true )
				&& ( cbMCHideInvalid.Checked ? Int32.Parse( e.Players ) < 600 : true );
			cc.Filter = e =>
				e.Name.IndexOf( txtCCSearch.Text, StringComparison.OrdinalIgnoreCase ) >= 0
				&& ( cbCCHideEmpty.Checked ? e.Players[0] != '0' : true );
			KeyPreview = true;
			KeyDown += KeyDownHandler;
		}

		void KeyDownHandler(object sender, KeyEventArgs e) {
			if( e.KeyCode != Keys.Enter ) return;
			
			if( tabs.SelectedTab == tabLocal ) {
				BtnLanConnectClick( null, null );
			} else if( tabs.SelectedTab == tabMinecraftNet ) {
				if( tabMC.SelectedTab == tabMCSignIn )
					mc.DoSignIn();
				else if( tabMC.SelectedTab == tabMCServers )
					mc.ConnectToServer();
			} else if( tabs.SelectedTab == tabClassicubeNet ) {
				if( tabCC.SelectedTab == tabCCSignIn )
					cc.DoSignIn();
				else if( tabCC.SelectedTab == tabCCServers )
					cc.ConnectToServer();
			}
			e.Handled = true;
		}

		GameState mc, cc;
		
		void DisplayResourcesDialog( object sender, EventArgs e ) {
			// TODO: async fetching
			ResourceFetcher fetcher = new ResourceFetcher();
			if( !fetcher.CheckAllResourcesExist() ) {
				DialogResult result = MessageBox.Show(
					"Some required resources weren't found. Would you like to download them now?", "Missing resources",
					MessageBoxButtons.OKCancel );
				if( result == DialogResult.OK ) {
					fetcher.Run( this );
					Text = AppName;
				}
			}
		}

		void SetTooltips() {
			toolTip.SetToolTip( lblLanUser, "The username to use when connecting to the local server. " + Environment.NewLine +
			                   "(this doesn't have to be your minecraft.net or classicube.net username)" );
			toolTip.SetToolTip( lblLanIP, "The IP address of the local server (i.e. either LAN or on the same computer)" + Environment.NewLine +
			                   "Valid IPv4 local addresses: 127.0.0.1, 192.168.*.*, 10.*.*.*, and 172.16.*.* to 172.31.*.*"  + Environment.NewLine +
			                   "Valid IPv6 local addresses: ::1, fd**:****:****:: and fd**:****:****:****:****:****:****:****" );
			toolTip.SetToolTip( lblLanPort, "The port number of the local server." + Environment.NewLine +
			                   "Must be greater or equal to 0, and less than 65536" );
			toolTip.SetToolTip( cbLocalSkinServerCC, "If checked, skins are downloaded from classicube.net instead of minecraft.net." );
			toolTip.IsBalloon = true;
		}
		
		void MainFormResizeEnd( object sender, EventArgs e ) {
			AdjustTabs();
		}
		
		void AdjustTabs() {
			tabs.Width = ClientSize.Width + 6;
			tabs.Height = ClientSize.Height + 3;
			tabMC.Width = tabCC.Width = tabs.Width;
			tabMC.Height = tabCC.Height = tabs.SelectedTab.Size.Height + 10;
			
			if( tblMCServers.IsHandleCreated ) {
				AdjustTablePos( tblMCServers, btnMCConnect );
			}
			if( tblCCServers.IsHandleCreated ) {
				AdjustTablePos( tblCCServers, btnCCConnect );
			}
		}
		
		void AdjustTablePos( Control control, Control connectButton ) {
			Point formLoc = PointToClient( control.Parent.PointToScreen( control.Location ) );
			control.Width = ClientSize.Width - formLoc.X;
			control.Height = ClientSize.Height - formLoc.Y - connectButton.Height - 5;
		}
		
		#region Local tab
		
		bool IsPrivateIP( IPAddress address ) {
			if( IPAddress.IsLoopback( address ) ) return true; // 127.0.0.1 or 0000:0000:0000:0000:0000:0000:0000:0001
			if( address.AddressFamily == AddressFamily.InterNetwork ) {
				byte[] bytes = address.GetAddressBytes();
				if( bytes[0] == 192 && bytes[1] == 168 ) return true; // 192.168.0.0 to 192.168.255.255
				if( bytes[0] == 10 ) return true; // 10.0.0.0 to 10.255.255.255
				if( bytes[0] == 172 && bytes[1] >= 16 && bytes[1] <= 31 ) return true;
			} else if( address.AddressFamily == AddressFamily.InterNetworkV6 ) {
				byte[] bytes = address.GetAddressBytes();
				if( bytes[0] == 0xFD ) return true; // fd00:0000:0000:0000:xxxx:xxxx:xxxx:xxxx
			}
			return false;
		}

		void BtnLanConnectClick( object sender, System.EventArgs e ) {
			IPAddress address;
			if( !IPAddress.TryParse( txtLanIP.Text, out address ) ) {
				MessageBox.Show( "Invalid IP address specified." );
				return;
			}
			if( !IsPrivateIP( address ) ) {
				MessageBox.Show( "Given IP address is not a local LAN address." );
				return;
			}
			
			ushort port;
			if( !UInt16.TryParse( txtLanPort.Text, out port ) ) {
				MessageBox.Show( "Invalid port specified." );
				return;
			}
			if( String.IsNullOrEmpty( txtLanUser.Text ) ) {
				MessageBox.Show( "Please enter a username." );
				return;
			}

			GameStartData data = new GameStartData( txtLanUser.Text, "lan",
			                                       txtLanIP.Text, txtLanPort.Text );
			StartClient( data, cbLocalSkinServerCC.Checked );
		}
		
		#endregion
		
		NameComparer mcNameComparer = new NameComparer( 0 );
		NumericalComparer mcPlayersComparer = new NumericalComparer( 1 );
		NumericalComparer mcMaxPlayersComparer = new NumericalComparer( 2 );
		UptimeComparer mcUptimeComparer = new UptimeComparer( 3 );
		void tblMCServersColumnClick( object sender, ColumnClickEventArgs e ) {
			if( e.Column == 0 ) {
				mcNameComparer.Invert = !mcNameComparer.Invert;
				tblMCServers.ListViewItemSorter = mcNameComparer;
			} else if( e.Column == 1 ) {
				mcPlayersComparer.Invert = !mcPlayersComparer.Invert;
				tblMCServers.ListViewItemSorter = mcPlayersComparer;
			} else if( e.Column == 2 ) {
				mcMaxPlayersComparer.Invert = !mcMaxPlayersComparer.Invert;
				tblMCServers.ListViewItemSorter = mcMaxPlayersComparer;
			} else if( e.Column == 3 ) {
				mcUptimeComparer.Invert = !mcUptimeComparer.Invert;
				tblMCServers.ListViewItemSorter = mcUptimeComparer;
			}
			tblMCServers.Sort();
		}
		
		void txtMCSearchTextChanged( object sender, EventArgs e ) { mc.FilterList(); }
		
		void cbMCHideEmptyCheckedChanged( object sender, EventArgs e ) { mc.FilterList(); }
		
		void cbMCHideInvalidCheckedChanged(object sender, EventArgs e ) { mc.FilterList(); }
		
		void tblMCServersClick( object sender, EventArgs e ) { mc.ServerTableClick(); }
		
		void txtMCHashTextChanged( object sender, EventArgs e ) { mc.HashChanged(); }
		
		void btnMCConnectClick( object sender, EventArgs e ) { mc.ConnectToServer(); }
		
		void btnMCSignInClick( object sender, EventArgs e ) { mc.DoSignIn(); }
		

		NameComparer ccNameComparer = new NameComparer( 0 );
		NumericalComparer ccPlayersComparer = new NumericalComparer( 1 );
		NumericalComparer ccMaxPlayersComparer = new NumericalComparer( 2 );
		void tblCCServersColumnClick( object sender, ColumnClickEventArgs e ) {
			if( e.Column == 0 ) {
				ccNameComparer.Invert = !ccNameComparer.Invert;
				tblCCServers.ListViewItemSorter = ccNameComparer;
			} else if( e.Column == 1 ) {
				ccPlayersComparer.Invert = !ccPlayersComparer.Invert;
				tblCCServers.ListViewItemSorter = ccPlayersComparer;
			} else if( e.Column == 2 ) {
				ccMaxPlayersComparer.Invert = !ccMaxPlayersComparer.Invert;
				tblCCServers.ListViewItemSorter = ccMaxPlayersComparer;
			} else if( e.Column == 3 ) {
				// NOTE: servers page doesn't give up time.
			}
			tblCCServers.Sort();
		}
		
		void txtCCSearchTextChanged( object sender, EventArgs e ) { cc.FilterList(); }
		
		void cbCCHideEmptyCheckedChanged( object sender, EventArgs e ) { cc.FilterList(); }
		
		void tblCCServersClick( object sender, EventArgs e ) { cc.ServerTableClick(); }
		
		void txtCCHashTextChanged( object sender, EventArgs e ) { cc.HashChanged(); }
		
		void btnCCConnectClick( object sender, EventArgs e ) { cc.ConnectToServer(); }
		
		void btnCCSignInClick( object sender, EventArgs e ) { cc.DoSignIn(); }
		
		
		static string missingExeMessage = "Failed to start ClassicalSharp. (classicalsharp.exe was not found)"
			+ Environment.NewLine + Environment.NewLine +
			"This application is only the launcher, it is not the actual client. " +
			"Please place the launcher in the same directory as the client (classicalsharp.exe).";
		
		internal static void StartClient( GameStartData data, bool classicubeSkins ) {
			string skinServer = classicubeSkins ? "http://www.classicube.net/static/skins/" : "http://s3.amazonaws.com/MinecraftSkins/";
			string args = data.Username + " " + data.Mppass + " " +
				data.Ip + " " + data.Port + " " + skinServer;
			System.Diagnostics.Debug.WriteLine( "starting..." + args );
			Process process = null;
			
			try {
				if( Type.GetType( "Mono.Runtime" ) != null ) {
					process = Process.Start( "mono", "\"ClassicalSharp.exe\" " + args );
				} else {
					process = Process.Start( "ClassicalSharp.exe", args );
				}
			} catch( Win32Exception ex ) {
				if( ex.Message.Contains( "The system cannot find the file specified" ) ) {
					MessageBox.Show( missingExeMessage );
				} else {
					throw;
				}
			}
		}
	}
}