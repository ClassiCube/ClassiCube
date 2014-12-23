using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Diagnostics;
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
			tblMCServers.HandleCreated += TblMcServersHandleCreated;
			tblCCServers.HandleCreated += TblCcServersHandleCreated;
			// hide tabs at start
			tabMC.TabPages.Remove( tabMCServers );
			tabMC.TabPages.Remove( tabMCServer );
			tabCC.TabPages.Remove( tabCCServers );
			tabCC.TabPages.Remove( tabCCServer );
		}
		
		delegate void Action<in T1, in T2>( T1 arg1, T2 arg2 );

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
		
		void TblMcServersHandleCreated( object sender, EventArgs e ) {
			AdjustTablePos( tblMCServers );
		}
		
		void TblCcServersHandleCreated( object sender, EventArgs e ) {
			AdjustTablePos( tblCCServers );
		}
		
		void AdjustTablePos( Control control ) {
			var formLoc = PointToClient( control.Parent.PointToScreen( control.Location ) );
			control.Width = ClientSize.Width - formLoc.X;
			control.Height = ClientSize.Height - formLoc.Y;
		}
		
		static readonly string MissingExeMessage = "Failed to start ClassicalSharp. (classicalsharp.exe was not found)"
			+ Environment.NewLine + Environment.NewLine +
			"This application is only the launcher, it is not the actual client. " +
			"Please place the launcher in the same directory as the client (classicalsharp.exe).";
		
		static void StartClient( GameStartData data, bool classicubeSkins ) {
			var skinServer = classicubeSkins ? "http://www.classicube.net/static/skins/" : "http://s3.amazonaws.com/MinecraftSkins/";
			var args = data.Username + " " + data.Mppass + " " +
				data.Ip + " " + data.Port + " " + skinServer;
			Log( "starting..." + args );
			Process process = null;
			try {
				process = Process.Start( "classicalsharp.exe", args );
			} catch( Win32Exception ex ) {
				if( ex.Message.Contains( "The system cannot find the file specified" ) ) {
					MessageBox.Show( MissingExeMessage );
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
			Debug.WriteLine( text );
		}
		
		static void DisplayWebException( WebException ex, string action, string host, Action<int, string> target ) {
			if( ex.Status == WebExceptionStatus.Timeout ) {
				var text = "Failed to " + action + ":" +
					Environment.NewLine + "Timed out while connecting to " + host + ", it may be down.";
				target( 0, text );
			} else if( ex.Status == WebExceptionStatus.ProtocolError ) {
				var response = (HttpWebResponse)ex.Response;
				var errorCode = (int)response.StatusCode;
				var description = response.StatusDescription;
				var text = "Failed to " + action + ":" +
					Environment.NewLine + host + " returned: (" + errorCode + ") " + description;
				target( 0, text );
			} else if( ex.Status == WebExceptionStatus.NameResolutionFailure ) {
				var text = "Failed to " + action + ":" +
					Environment.NewLine + "Unable to resolve " + host + ", you may not be connected to the internet.";
				target( 0, text );
			} else {
				var text = "Failed to " + action + ":" +
					Environment.NewLine + ex.Status;
				target( 0, text );
			}
		}
		
		#region Local tab
		
		bool IsPrivateIp( IPAddress address ) {
			if( IPAddress.IsLoopback( address ) ) return true; // 127.0.0.1 or 0000:0000:0000:0000:0000:0000:0000:0001
			if( address.AddressFamily == AddressFamily.InterNetwork ) {
				var bytes = address.GetAddressBytes();
				if( bytes[0] == 192 && bytes[1] == 168 ) return true; // 192.168.0.0 to 192.168.255.255
				if( bytes[0] == 10 ) return true; // 10.0.0.0 to 10.255.255.255
				if( bytes[0] == 172 && bytes[1] >= 16 && bytes[1] <= 31 ) return true;
			} else if( address.AddressFamily == AddressFamily.InterNetworkV6 ) {
				var bytes = address.GetAddressBytes();
				if( bytes[0] == 0xFD ) return true; // fd00:0000:0000:0000:xxxx:xxxx:xxxx:xxxx
			}
			return false;
		}

		void BtnLanConnectClick( object sender, EventArgs e ) {
			IPAddress address;
			if( !IPAddress.TryParse( txtLanIP.Text, out address ) ) {
				MessageBox.Show( "Invalid IP address specified." );
				return;
			}
			if( !IsPrivateIp( address ) ) {
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

			var data = new GameStartData( txtLanUser.Text, "lan",
			                                       txtLanIP.Text, txtLanPort.Text );
			StartClient( data, cbLocalSkinServerCC.Checked );
		}
		
		#endregion
		
		
		#region minecraft.net tab

	    readonly MinecraftSession _mcSession = new MinecraftSession();
		List<ServerListEntry> _mcServers = new List<ServerListEntry>();

	    readonly NameComparer _mcNameComparer = new NameComparer( 0 );
	    readonly NumericalComparer _mcPlayersComparer = new NumericalComparer( 1 );
	    readonly NumericalComparer _mcMaxPlayersComparer = new NumericalComparer( 2 );
	    readonly UptimeComparer _mcUptimeComparer = new UptimeComparer( 3 );

		void TblMcServersColumnClick( object sender, ColumnClickEventArgs e ) {
			if( e.Column == 0 ) {
				_mcNameComparer.Invert = !_mcNameComparer.Invert;
				tblMCServers.ListViewItemSorter = _mcNameComparer;
			} else if( e.Column == 1 ) {
				_mcPlayersComparer.Invert = !_mcPlayersComparer.Invert;
				tblMCServers.ListViewItemSorter = _mcPlayersComparer;
			} else if( e.Column == 2 ) {
				_mcMaxPlayersComparer.Invert = !_mcMaxPlayersComparer.Invert;
				tblMCServers.ListViewItemSorter = _mcMaxPlayersComparer;
			} else if( e.Column == 3 ) {
				_mcUptimeComparer.Invert = !_mcUptimeComparer.Invert;
				tblMCServers.ListViewItemSorter = _mcUptimeComparer;
			}
			tblMCServers.Sort();
		}
		
		void McFilterList() {
			tblMCServers.Items.Clear();
			if( _mcServers.Count == 0 ) return;
			var sorter = tblMCServers.ListViewItemSorter;
			tblMCServers.ListViewItemSorter = null;
			
			tblMCServers.BeginUpdate();
			Predicate<ServerListEntry> filter = e =>
				// NOTE: using ToLower().Contains() allocates too many unecessary strings.
				e.Name.IndexOf( txtMCSearch.Text, StringComparison.OrdinalIgnoreCase ) >= 0
				&& ( !cbMCHideEmpty.Checked || e.Players[0] != '0' )
				&& ( !cbMCHideInvalid.Checked || Int32.Parse( e.Players ) < 600 );
			
			foreach (var entry in _mcServers) {
			    if (!filter(entry)) continue;
			    string[] row = { entry.Name, entry.Players, entry.MaximumPlayers, entry.Uptime, entry.Hash };
			    tblMCServers.Items.Add( new ListViewItem( row ) );
			}
			if( sorter != null ) {
				tblMCServers.ListViewItemSorter = sorter;
				tblMCServers.Sort();
			}
			tblMCServers.EndUpdate();
		}
		
		void TxtMcSearchTextChanged( object sender, EventArgs e ) {
			McFilterList();
		}
		
		void CbMcHideEmptyCheckedChanged( object sender, EventArgs e ) {
			McFilterList();
		}
		
		void CbMcHideInvalidCheckedChanged(object sender, EventArgs e ) {
			McFilterList();
		}
		
		void TblMcServersDoubleClick( object sender, EventArgs e ) {
			var mousePos = tblMCServers.PointToClient( MousePosition );
			var hitTest = tblMCServers.HitTest( mousePos );

		    if (hitTest.Item == null) 
                return;

		    txtMCHash.Text = hitTest.Item.SubItems[4].Text;
		    tabMC.SelectedIndex = 2;
		}
		
		void TxtMcHashTextChanged( object sender, EventArgs e ) {
			var hash = txtMCHash.Text;
			lblMCPublicName.Text = "No public name";

			foreach (var entry in _mcServers) {
			    if (hash != entry.Hash) 
                    continue;

			    lblMCPublicName.Text = entry.Name;
			    break;
			}
		}
		
		void BtnMcConnectClick( object sender, EventArgs e ) {
			GameStartData data;
			try {
				data = _mcSession.GetConnectInfo( txtMCHash.Text );
			} catch( WebException ex ) {
				DisplayWebException( ex, "retrieve server info", "minecraft.net",
					(p, text) => MessageBox.Show( text ) );
				return;
			}
			StartClient( data, false );
		}
		
		void BtnMcSignInClick( object sender, EventArgs e ) {
			if( String.IsNullOrEmpty( txtMCUser.Text ) ) {
				MessageBox.Show( "Please enter a username." );
				return;
			}
			if( String.IsNullOrEmpty( txtMCPassword.Text ) ) {
				MessageBox.Show( "Please enter a password." );
				return;
			}
			
			if( _mcSession.Username != null ) {
				_mcSession.ResetSession();
				tblMCServers.Items.Clear();
				_mcServers.Clear();
			}
			btnMCSignIn.Enabled = false;
			tabMC.TabPages.Remove( tabMCServers );
			tabMC.TabPages.Remove( tabMCServer );
		    _mcLoginThread = new Thread(McLoginAsync) {Name = "Launcher.McLoginAsync", IsBackground = true};
		    _mcLoginThread.Start();
		}
		
		Thread _mcLoginThread;
		void McLoginAsync() {
			SetMcStatus( 0, "Signing in.." );
			try {
				_mcSession.Login( txtMCUser.Text, txtMCPassword.Text );
			} catch( WebException ex ) {
				_mcSession.Username = null;
				McLoginEnd( false );
				DisplayWebException( ex, "sign in", "minecraft.net", SetMcStatus );
				return;
			} catch( InvalidOperationException ex ) {
				SetMcStatus( 0, "Failed to sign in" );
				_mcSession.Username = null;
				McLoginEnd( false );
				var text = "Failed to sign in: " + Environment.NewLine + ex.Message;
				SetMcStatus( 0, text );
				return;
			}
			
			SetMcStatus( 50, "Retrieving public servers list.." );
			try {
				_mcServers = _mcSession.GetPublicServers();
			} catch( WebException ex ) {
				_mcServers = new List<ServerListEntry>();
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

	    private readonly ClassicubeInteraction _ccSession = new ClassicubeInteraction("", "");
        private ClassicubeServer[] _ccServers = new ClassicubeServer[0];
	    private int _currentItem;

	    readonly NameComparer _ccNameComparer = new NameComparer( 0 );
	    readonly NumericalComparer _ccPlayersComparer = new NumericalComparer( 1 );
	    readonly NumericalComparer _ccMaxPlayersComparer = new NumericalComparer( 2 );

		void TblCcServersColumnClick( object sender, ColumnClickEventArgs e ) {
		    switch (e.Column) {
		        case 0:
		            _ccNameComparer.Invert = !_ccNameComparer.Invert;
		            tblCCServers.ListViewItemSorter = _ccNameComparer;
		            break;
		        case 1:
		            _ccPlayersComparer.Invert = !_ccPlayersComparer.Invert;
		            tblCCServers.ListViewItemSorter = _ccPlayersComparer;
		            break;
		        case 2:
		            _ccMaxPlayersComparer.Invert = !_ccMaxPlayersComparer.Invert;
		            tblCCServers.ListViewItemSorter = _ccMaxPlayersComparer;
		            break;
		        case 3:
		            // NOTE: servers page doesn't give up time.
		            break;
		    }

		    tblCCServers.Sort();
		}

	    void CcFilterList() {
            tblCCServers.Items.Clear();

            if (_ccServers.Length == 0)
                return;

            var sorter = tblCCServers.ListViewItemSorter;
            tblCCServers.ListViewItemSorter = null;

            tblCCServers.BeginUpdate();

            Predicate<ClassicubeServer> filter = e =>
                e.Name.IndexOf(txtCCSearch.Text, StringComparison.OrdinalIgnoreCase) >= 0
                && (!cbCCHideEmpty.Checked || e.OnlinePlayers != 0);

            foreach (var entry in _ccServers) {
                if (!filter(entry))
                    continue;

                string[] row = { entry.Name, entry.OnlinePlayers.ToString(), entry.MaxPlayers.ToString(), entry.Uptime.ToString(), entry.Hash };
                tblCCServers.Items.Add(new ListViewItem(row));
            }

            if (sorter != null) {
                tblCCServers.ListViewItemSorter = sorter;
                tblCCServers.Sort();
            }

            tblCCServers.EndUpdate();
		}
		
		void TxtCcSearchTextChanged( object sender, EventArgs e ) {
			CcFilterList();
		}
		
		void CbCcHideEmptyCheckedChanged( object sender, EventArgs e ) {
			CcFilterList();
		}
		
		void TblCcServersDoubleClick( object sender, EventArgs e ) {
		    if (tblCCServers.SelectedIndices.Count == 0)
		        return;

		    var item = tblCCServers.SelectedIndices[0];
		    var entry = _ccServers[item];

		    txtCCHash.Text = entry.Hash;
		    _currentItem = item;
		    tabCC.SelectedIndex = 2;
		}
		
		void BtnCcConnectClick( object sender, EventArgs e ) {
			GameStartData data;

			try {
                data = new GameStartData(_ccSession.Username, _ccServers[_currentItem].Mppass, _ccServers[_currentItem].Ip, _ccServers[_currentItem].Port.ToString());
			} catch( WebException ex ) {
				DisplayWebException( ex, "retrieve server info", "classicube.net",
					(p, text) => MessageBox.Show( text ) );
				return;
			}
			StartClient( data, true );
		}
		
		void BtnCcSignInClick( object sender, EventArgs e ) {
			if( String.IsNullOrEmpty( txtCCUser.Text ) ) {
				MessageBox.Show( "Please enter a username." );
				return;
			}
			if( String.IsNullOrEmpty( txtCCPassword.Text ) ) {
				MessageBox.Show( "Please enter a password." );
				return;
			}
			
			if( String.IsNullOrEmpty(_ccSession.Username)) {
			    _ccSession.Username = txtCCUser.Text;
			    _ccSession.Password = txtCCPassword.Text;
				tblCCServers.Items.Clear();
				_ccServers = new ClassicubeServer[0];
			}

			btnCCSignIn.Enabled = false;
			tabCC.TabPages.Remove( tabCCServers );
			tabCC.TabPages.Remove( tabCCServer );
		    _ccLoginThread = new Thread(CcLoginAsync) {Name = "Launcher.CcLoginAsync", IsBackground = true};
		    _ccLoginThread.Start();
		}
		
		Thread _ccLoginThread;
		void CcLoginAsync() {
			SetCcStatus( 0, "Signing in.." );

			try {
				_ccSession.Login();
			} catch( WebException ex ) {
				_ccSession.Username = null;
				CcLoginEnd( false );
				DisplayWebException( ex, "sign in", "classicube.net", SetCcStatus );
				return;
			} catch( InvalidOperationException ex ) {
				SetCcStatus( 0, "Failed to sign in" );
				_ccSession.Username = null;
				CcLoginEnd( false );
				var text = "Failed to sign in: " + Environment.NewLine + ex.Message;
				SetCcStatus( 0, text );
				return;
			}
			
			SetCcStatus( 50, "Retrieving public servers list.." );

			try {
			    _ccServers = _ccSession.GetAllServers();
			} catch( WebException ex ) {
				_ccServers = new ClassicubeServer[0];
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