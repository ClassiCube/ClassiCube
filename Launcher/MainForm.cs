using System;
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
			tabCC.TabPages.Remove( tabCCServers );
			Shown += DisplayResourcesDialog;

			cc = new GameState() { Progress = prgCCStatus, Status = lblCCStatus,
				Username = txtCCUser, Password = txtCCPassword, HostServer = "classicube.net",
				Session = new ClassicubeSession(), SignInButton = btnCCSignIn, Tab = tabCC,
				ServersTab = tabCCServers, ServersTable = tblCCServers, Hash = txtCCHash,
				form = this };
			
			cc.Filter = e =>
				e.Name.IndexOf( txtCCSearch.Text, StringComparison.OrdinalIgnoreCase ) >= 0
				&& ( cbCCHideEmpty.Checked ? e.Players[0] != '0' : true );
			KeyPreview = true;
			KeyDown += KeyDownHandler;
			LoadSavedInfo();
		}

		void KeyDownHandler(object sender, KeyEventArgs e) {
			if( e.KeyCode != Keys.Enter ) return;
			
			if( tabs.SelectedTab == tabDC ) {
				BtnDCconnectClick( null, null );
			} else if( tabs.SelectedTab == tabClassicubeNet ) {
				if( tabCC.SelectedTab == tabCCSignIn )
					cc.DoSignIn();
				else if( tabCC.SelectedTab == tabCCServers )
					cc.ConnectToServer();
			}
			e.Handled = true;
		}
		GameState cc;
		
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
			tabCC.Width = tabs.Width;
			tabCC.Height = tabs.SelectedTab.Size.Height + 10;
			
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
		
		void LoadSavedInfo() {
			if( !Options.Load() ) 
				return;
			txtDCuser.Text = Options.Get( "launcher-username" ) ?? "";
			txtDCip.Text = Options.Get( "launcher-ip" ) ?? "127.0.0.1";
			txtDCport.Text = Options.Get( "launcher-port" ) ?? "25565";
			cbDCccskins.Checked = Options.GetBool( "launcher-ccskins", false );
			txtCCUser.Text = Options.Get( "launcher-cc-username" ) ?? "";

			IPAddress address;
			if( !IPAddress.TryParse( txtDCip.Text, out address ) )
				txtDCip.Text = "127.0.0.1";
			ushort port;
			if( !UInt16.TryParse( txtDCport.Text, out port ) )
				txtDCport.Text = "25565";
			
			string mppass = Options.Get( "launcher-mppass" ) ?? null;
			mppass = Secure.Decode( mppass, txtDCuser.Text );
			if( mppass != null )
				txtDCmppass.Text = mppass;
			
			mppass = Options.Get( "launcher-cc-password" ) ?? null;
			mppass = Secure.Decode( mppass, txtCCUser.Text );
			if( mppass != null )
				txtCCPassword.Text = mppass;
		}
		#endregion

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
			UpdateResumeInfo( data, classicubeSkins );
			
			if( !File.Exists( "ClassicalSharp.exe" ) ) {
				MessageBox.Show( missingExeMessage );
				return;
			}
			
			if( Type.GetType( "Mono.Runtime" ) != null ) {
				process = Process.Start( "mono", "\"ClassicalSharp.exe\" " + args );
			} else {
				process = Process.Start( "ClassicalSharp.exe", args );
			}
		}
		
		internal static void UpdateResumeInfo( GameStartData data, bool classiCubeSkins ) {
			// If the client has changed some settings in the meantime, make sure we keep the changes
			if( !Options.Load() )
				return;
			
			Options.Set( "launcher-username", data.Username );
			Options.Set( "launcher-ip", data.Ip );
			Options.Set( "launcher-port", data.Port );
			Options.Set( "launcher-mppass", Secure.Encode( data.Mppass, data.Username ) );
			Options.Set( "launcher-ccskins", classiCubeSkins );
			Options.Save();
		}
	}
}