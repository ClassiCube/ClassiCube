using System;
using System.Collections;
using System.Collections.Generic;
using System.ComponentModel;
using System.Diagnostics;
using System.Drawing;
using System.Net;
using System.Net.Sockets;
using System.Threading;
using System.Windows.Forms;

namespace Launcher {
	
	public partial class MainForm : Form {
		
		public MainForm() {
			InitializeComponent();
			AdjustTabs();
			SetTooltips();
			tblMCServers.HandleCreated += tblMCServersHandleCreated;
			tblCCServers.HandleCreated += tblCCServersHandleCreated;
			// hide tabs at start
			tabMC.TabPages.Remove( tabMCServers );
			tabMC.TabPages.Remove( tabMCServer );
			tabCC.TabPages.Remove( tabCCServers );
			tabCC.TabPages.Remove( tabCCServer );
		}
		
		delegate void Action<T1, T2>( T1 arg1, T2 arg2 );

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
			tabMC.Width = tabCC.Width = tabs.SelectedTab.Size.Width + 10;
			tabMC.Height = tabCC.Height = tabs.SelectedTab.Size.Height + 10;
			
			if( tblMCServers.IsHandleCreated ) {
				AdjustTablePos( tblMCServers );
			}
			if( tblCCServers.IsHandleCreated ) {
				AdjustTablePos( tblCCServers );
			}
		}
		
		void tblMCServersHandleCreated( object sender, EventArgs e ) {
			AdjustTablePos( tblMCServers );
		}
		
		void tblCCServersHandleCreated( object sender, EventArgs e ) {
			AdjustTablePos( tblCCServers );
		}
		
		void AdjustTablePos( Control control ) {
			Point formLoc = PointToClient( control.Parent.PointToScreen( control.Location ) );
			control.Width = ClientSize.Width - formLoc.X;
			control.Height = ClientSize.Height - formLoc.Y;
		}
		
		static string missingExeMessage = "Failed to start ClassicalSharp. (classicalsharp.exe was not found)"
			+ Environment.NewLine + Environment.NewLine +
			"This application is only the launcher, it is not the actual client. " +
			"Please place the launcher in the same directory as the client (classicalsharp.exe).";
		
		static void StartClient( GameStartData data, bool classicubeSkins ) {
			string skinServer = classicubeSkins ? "http://www.classicube.net/static/skins/" : "http://s3.amazonaws.com/MinecraftSkins/";
			string args = data.Username + " " + data.Mppass + " " +
				data.Ip + " " + data.Port + " " + skinServer;
			Log( "starting..." + args );
			Process process = null;
			try {
				process = Process.Start( "classicalsharp.exe", args );
			} catch( Win32Exception ex ) {
				if( ex.Message.Contains( "The system cannot find the file specified" ) ) {
					MessageBox.Show( missingExeMessage );
				} else {
					throw;
				}
			} finally {
				if( process != null )  {
					process.Dispose();
				}
			}
		}
		
		static void Log( string text ) {
			System.Diagnostics.Debug.WriteLine( text );
		}
		
		static void DisplayWebException( WebException ex, string action, string host, Action<int, string> target ) {
			if( ex.Status == WebExceptionStatus.Timeout ) {
				string text = "Failed to " + action + ":" +
					Environment.NewLine + "Timed out while connecting to " + host + ", it may be down.";
				target( 0, text );
			} else if( ex.Status == WebExceptionStatus.ProtocolError ) {
				HttpWebResponse response = (HttpWebResponse)ex.Response;
				int errorCode = (int)response.StatusCode;
				string description = response.StatusDescription;
				string text = "Failed to " + action + ":" +
					Environment.NewLine + host + " returned: (" + errorCode + ") " + description;
				target( 0, text );
			} else if( ex.Status == WebExceptionStatus.NameResolutionFailure ) {
				string text = "Failed to " + action + ":" +
					Environment.NewLine + "Unable to resolve " + host + ", you may not be connected to the internet.";
				target( 0, text );
			} else {
				string text = "Failed to " + action + ":" +
					Environment.NewLine + ex.Status;
				target( 0, text );
			}
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
		
		
		#region minecraft.net tab
		
		MinecraftSession mcSession = new MinecraftSession();
		List<ServerListEntry> mcServers = new List<ServerListEntry>();
		
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
		
		void McFilterList() {
			tblMCServers.Items.Clear();
			if( mcServers.Count == 0 ) return;
			IComparer sorter = tblMCServers.ListViewItemSorter;
			tblMCServers.ListViewItemSorter = null;
			
			tblMCServers.BeginUpdate();
			Predicate<ServerListEntry> filter = e =>
				// NOTE: using ToLower().Contains() allocates too many unecessary strings.
				e.Name.IndexOf( txtMCSearch.Text, StringComparison.OrdinalIgnoreCase ) >= 0
				&& ( cbMCHideEmpty.Checked ? e.Players[0] != '0' : true )
				&& ( cbMCHideInvalid.Checked ? Int32.Parse( e.Players ) < 600 : true );
			
			for( int i = 0; i < mcServers.Count; i++ ) {
				ServerListEntry entry = mcServers[i];
				if( filter( entry ) ) {
					string[] row = { entry.Name, entry.Players, entry.MaximumPlayers, entry.Uptime, entry.Hash };
					tblMCServers.Items.Add( new ListViewItem( row ) );
				}
			}
			if( sorter != null ) {
				tblMCServers.ListViewItemSorter = sorter;
				tblMCServers.Sort();
			}
			tblMCServers.EndUpdate();
		}
		
		void txtMCSearchTextChanged( object sender, EventArgs e ) {
			McFilterList();
		}
		
		void cbMCHideEmptyCheckedChanged( object sender, EventArgs e ) {
			McFilterList();
		}
		
		void cbMCHideInvalidCheckedChanged(object sender, EventArgs e ) {
			McFilterList();
		}
		
		void tblMCServersDoubleClick( object sender, EventArgs e ) {
			Point mousePos = tblMCServers.PointToClient( MousePosition );
			ListViewHitTestInfo hitTest = tblMCServers.HitTest( mousePos );
			if( hitTest != null && hitTest.Item != null ) {
				txtMCHash.Text = hitTest.Item.SubItems[4].Text;
				tabMC.SelectedIndex = 2;
			}
		}
		
		void txtMCHashTextChanged( object sender, EventArgs e ) {
			string hash = txtMCHash.Text;
			lblMCPublicName.Text = "No public name";
			for( int i = 0; i < mcServers.Count; i++ ) {
				ServerListEntry entry = mcServers[i];
				if( hash == entry.Hash ) {
					lblMCPublicName.Text = entry.Name;
					break;
				}
			}
		}
		
		void btnMCConnectClick( object sender, EventArgs e ) {
			GameStartData data = null;
			try {
				data = mcSession.GetConnectInfo( txtMCHash.Text );
			} catch( WebException ex ) {
				DisplayWebException( ex, "retrieve server info", "minecraft.net",
					(p, text) => MessageBox.Show( text ) );
				return;
			}
			StartClient( data, false );
		}
		
		void btnMCSignInClick( object sender, EventArgs e ) {
			if( String.IsNullOrEmpty( txtMCUser.Text ) ) {
				MessageBox.Show( "Please enter a username." );
				return;
			}
			if( String.IsNullOrEmpty( txtMCPassword.Text ) ) {
				MessageBox.Show( "Please enter a password." );
				return;
			}
			
			if( mcSession.Username != null ) {
				mcSession.ResetSession();
				tblMCServers.Items.Clear();
				mcServers.Clear();
			}
			btnMCSignIn.Enabled = false;
			tabMC.TabPages.Remove( tabMCServers );
			tabMC.TabPages.Remove( tabMCServer );
			mcLoginThread = new Thread( McLoginAsync );
			mcLoginThread.Name = "Launcher.McLoginAsync";
			mcLoginThread.IsBackground = true;
			mcLoginThread.Start();
		}
		
		Thread mcLoginThread;
		void McLoginAsync() {
			SetMcStatus( 0, "Signing in.." );
			try {
				mcSession.Login( txtMCUser.Text, txtMCPassword.Text );
			} catch( WebException ex ) {
				mcSession.Username = null;
				McLoginEnd( false );
				DisplayWebException( ex, "sign in", "minecraft.net", SetMcStatus );
				return;
			} catch( InvalidOperationException ex ) {
				SetMcStatus( 0, "Failed to sign in" );
				mcSession.Username = null;
				McLoginEnd( false );
				string text = "Failed to sign in: " + Environment.NewLine + ex.Message;
				SetMcStatus( 0, text );
				return;
			}
			
			SetMcStatus( 50, "Retrieving public servers list.." );
			try {
				mcServers = mcSession.GetPublicServers();
			} catch( WebException ex ) {
				mcServers = new List<ServerListEntry>();
				McLoginEnd( false );
				DisplayWebException( ex, "retrieve servers list", "minecraft.net", SetMcStatus );
				return;
			}
			SetMcStatus( 100, "Done" );
			McLoginEnd( true );
		}
		
		void McLoginEnd( bool success ) {
			if( InvokeRequired ) {
				Invoke( (Action<bool>)McLoginEnd, success );
			} else {
				GC.Collect();
				if( success ) {
					tabMC.TabPages.Add( tabMCServers );
					tabMC.TabPages.Add( tabMCServer );
					McFilterList();
				}
				btnMCSignIn.Enabled = true;
			}
		}

		void SetMcStatus( int percentage, string text ) {
			if( InvokeRequired ) {
				Invoke( (Action<int, string>)SetMcStatus, percentage, text );
			} else {
				prgMCStatus.Value = percentage;
				lblMCStatus.Text = text;
			}
		}

		#endregion
		
		
		#region classicube.net tab
		
		ClassicubeSession ccSession = new ClassicubeSession();
		List<ServerListEntry> ccServers = new List<ServerListEntry>();
		
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
		
		void CcFilterList() {
			tblCCServers.Items.Clear();
			if( ccServers.Count == 0 ) return;
			IComparer sorter = tblCCServers.ListViewItemSorter;
			tblCCServers.ListViewItemSorter = null;
			
			tblCCServers.BeginUpdate();
			Predicate<ServerListEntry> filter = e =>
				e.Name.IndexOf( txtCCSearch.Text, StringComparison.OrdinalIgnoreCase ) >= 0
				&& ( cbCCHideEmpty.Checked ? e.Players[0] != '0' : true );
			
			for( int i = 0; i < ccServers.Count; i++ ) {
				ServerListEntry entry = ccServers[i];
				if( filter( entry ) ) {
					string[] row = { entry.Name, entry.Players, entry.MaximumPlayers, entry.Uptime, entry.Hash };
					tblCCServers.Items.Add( new ListViewItem( row ) );
				}
			}
			if( sorter != null ) {
				tblCCServers.ListViewItemSorter = sorter;
				tblCCServers.Sort();
			}
			tblCCServers.EndUpdate();
		}
		
		void txtCCSearchTextChanged( object sender, EventArgs e ) {
			CcFilterList();
		}
		
		void cbCCHideEmptyCheckedChanged( object sender, EventArgs e ) {
			CcFilterList();
		}
		
		void tblCCServersDoubleClick( object sender, EventArgs e ) {
			Point mousePos = tblCCServers.PointToClient( MousePosition );
			ListViewHitTestInfo hitTest = tblCCServers.HitTest( mousePos );
			if( hitTest != null && hitTest.Item != null ) {
				txtCCHash.Text = hitTest.Item.SubItems[4].Text;
				tabCC.SelectedIndex = 2;
			}
		}
		
		void txtCCHashTextChanged( object sender, EventArgs e ) {
			string hash = txtCCHash.Text;
			lblCCPublicName.Text = "No public name";
			for( int i = 0; i < ccServers.Count; i++ ) {
				ServerListEntry entry = ccServers[i];
				if( hash == entry.Hash ) {
					lblCCPublicName.Text = entry.Name;
					break;
				}
			}
		}
		
		void btnCCConnectClick( object sender, EventArgs e ) {
			GameStartData data = null;
			try {
				data = ccSession.GetConnectInfo( txtCCHash.Text );
			} catch( WebException ex ) {
				DisplayWebException( ex, "retrieve server info", "classicube.net",
					(p, text) => MessageBox.Show( text ) );
				return;
			}
			StartClient( data, true );
		}
		
		void btnCCSignInClick( object sender, EventArgs e ) {
			if( String.IsNullOrEmpty( txtCCUser.Text ) ) {
				MessageBox.Show( "Please enter a username." );
				return;
			}
			if( String.IsNullOrEmpty( txtCCPassword.Text ) ) {
				MessageBox.Show( "Please enter a password." );
				return;
			}
			
			if( ccSession.Username != null ) {
				ccSession.ResetSession();
				tblCCServers.Items.Clear();
				ccServers.Clear();
			}
			btnCCSignIn.Enabled = false;
			tabCC.TabPages.Remove( tabCCServers );
			tabCC.TabPages.Remove( tabCCServer );
			ccLoginThread = new Thread( CcLoginAsync );
			ccLoginThread.Name = "Launcher.CcLoginAsync";
			ccLoginThread.IsBackground = true;
			ccLoginThread.Start();
		}
		
		Thread ccLoginThread;
		void CcLoginAsync() {
			SetCcStatus( 0, "Signing in.." );
			try {
				ccSession.Login( txtCCUser.Text, txtCCPassword.Text );
			} catch( WebException ex ) {
				ccSession.Username = null;
				CcLoginEnd( false );
				DisplayWebException( ex, "sign in", "classicube.net", SetCcStatus );
				return;
			} catch( InvalidOperationException ex ) {
				SetCcStatus( 0, "Failed to sign in" );
				ccSession.Username = null;
				CcLoginEnd( false );
				string text = "Failed to sign in: " + Environment.NewLine + ex.Message;
				SetCcStatus( 0, text );
				return;
			}
			
			SetCcStatus( 50, "Retrieving public servers list.." );
			try {
				ccServers = ccSession.GetPublicServers();
			} catch( WebException ex ) {
				ccServers = new List<ServerListEntry>();
				CcLoginEnd( false );
				DisplayWebException( ex, "retrieve servers list", "classicube.net", SetCcStatus );
				return;
			}
			SetCcStatus( 100, "Done" );
			CcLoginEnd( true );
		}
		
		void CcLoginEnd( bool success ) {
			if( InvokeRequired ) {
				Invoke( (Action<bool>)CcLoginEnd, success );
			} else {
				GC.Collect();
				if( success ) {
					tabCC.TabPages.Add( tabCCServers );
					tabCC.TabPages.Add( tabCCServer );
					CcFilterList();
				}
				btnCCSignIn.Enabled = true;
			}
		}

		void SetCcStatus( int percentage, string text ) {
			if( InvokeRequired ) {
				Invoke( (Action<int, string>)SetCcStatus, percentage, text );
			} else {
				prgCCStatus.Value = percentage;
				lblCCStatus.Text = text;
			}
		}
		
		#endregion
	}
}