using System;
using System.ComponentModel;
using System.Diagnostics;
using System.Drawing;
using System.IO;
using System.Net;
using System.Windows.Forms;
using ClassicalSharp;

namespace Launcher {
	
	public partial class MainForm : Form {
		
		public const string AppName = "ClassicalSharp Launcher 0.95";
		
		public MainForm() {
			InitializeComponent();
			AdjustTabs();
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
			LoadResumeInfo();
		}

		void KeyDownHandler(object sender, KeyEventArgs e) {
			if( e.KeyCode != Keys.Enter ) return;
			
			if( tabs.SelectedTab == tabDC ) {
				BtnDCconnectClick( null, null );
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
		
		#region Direct connection tab

		void BtnDCconnectClick( object sender, EventArgs e ) {
			IPAddress address;
			if( !IPAddress.TryParse( txtDCip.Text, out address ) ) {
				MessageBox.Show( "Invalid IP address specified." );
				return;
			}
			
			ushort port;
			if( !UInt16.TryParse( txtDCport.Text, out port ) ) {
				MessageBox.Show( "Invalid port specified." );
				return;
			}
			if( String.IsNullOrEmpty( txtDCuser.Text ) ) {
				MessageBox.Show( "Please enter a username." );
				return;
			}
			
			string mppass = txtDCmppass.Text;
			if( String.IsNullOrEmpty( mppass ) ) mppass = "(none)";
			GameStartData data = new GameStartData( txtDCuser.Text, mppass,
			                                       txtDCip.Text, txtDCport.Text );
			StartClient( data, cbDCccskins.Checked );
		}
		
		void LoadResumeInfo() {
			try {
				Options.Load();
			} catch( IOException ) {
				return;
			}
			
			txtDCuser.Text = Options.Get( "launcher-username" ) ?? "";
			txtDCip.Text = Options.Get( "launcher-ip" ) ?? "127.0.0.1";
			txtDCport.Text = Options.Get( "launcher-port" ) ?? "25565";
			cbDCccskins.Checked = Options.GetBool( "launcher-ccskins", false );

			IPAddress address;
			if( !IPAddress.TryParse( txtDCip.Text, out address ) )
				txtDCip.Text = "127.0.0.1";
			ushort port;
			if( !UInt16.TryParse( txtDCport.Text, out port ) )
				txtDCport.Text = "25565";
			
			string mppass = Options.Get( "launcher-mppass" ) ?? null;
			mppass = DecodeMppass( mppass, txtDCuser.Text );
			if( mppass != null )
				txtDCmppass.Text = mppass;
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
			UpdateOptions( data, classicubeSkins );
			
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
		
		static string EncodeMppass( string mppass, string user ) {
			if( String.IsNullOrEmpty( mppass ) || String.IsNullOrEmpty( user ) ) return "";
			
			byte[] c = new byte[1 + mppass.Length];
			for( int i = 0; i < mppass.Length; i++ ) {
				c[i + 1] = (byte)( mppass[i] ^ user[i % user.Length] ^ 0x43 );
			}
			c[0] = 1; // format version
			// TODO: version 2 using CryptProtectData for Windows
			return Convert.ToBase64String( c );
		}
		
		static string DecodeMppass( string encoded, string user ) {
			if( String.IsNullOrEmpty( encoded ) || String.IsNullOrEmpty( user ) ) return null;
			
			byte[] data;
			try {
				data = Convert.FromBase64String( encoded );
			} catch( FormatException ) {
				return null;
			}
			if( data.Length == 0 || data[0] != 1 ) return null;
			
			char[] c = new char[data.Length - 1];
			for( int i = 0; i < c.Length; i++ ) {
				c[i] = (char)( data[i + 1] ^ user[i % user.Length] ^ 0x43 );
			}
			return new String( c );
		}
		
		internal static void UpdateOptions( GameStartData data, bool classiCubeSkins ) {
			Options.Set( "launcher-username", data.Username );
			Options.Set( "launcher-ip", data.Ip );
			Options.Set( "launcher-port", data.Port );
			Options.Set( "launcher-mppass", EncodeMppass( data.Mppass, data.Username ) );
			Options.Set( "launcher-ccskins", classiCubeSkins.ToString() );
			
			try {
				Options.Save();
			} catch( IOException ) {
			}
		}
	}
}