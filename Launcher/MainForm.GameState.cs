using System;
using System.Collections;
using System.Collections.Generic;
using System.Drawing;
using System.Net;
using System.Threading;
using System.Windows.Forms;

namespace Launcher {
	
	public delegate void Action<T1, T2>( T1 arg1, T2 arg2 );
	
	internal class GameState {
		public ProgressBar Progress;
		public Label Status;
		public TextBox Username, Password;
		public List<ServerListEntry> Servers;
		public string HostServer;
		public GameSession Session;
		public Button SignInButton;
		public TabControl Tab;
		public TabPage ServersTab;
		public ListView ServersTable;
		public TextBox Hash;
		public Predicate<ServerListEntry> Filter;
		public MainForm form;
		
		public void DoSignIn() {
			if( String.IsNullOrEmpty( Username.Text ) ) {
				MessageBox.Show( "Please enter a username." );
				return;
			}
			if( String.IsNullOrEmpty( Password.Text ) ) {
				MessageBox.Show( "Please enter a password." );
				return;
			}
			
			if( Session.Username != null ) {
				Session.ResetSession();
				ServersTable.Items.Clear();
				Servers.Clear();
			}
			SignInButton.Enabled = false;
			Tab.TabPages.Remove( ServersTab );
			Thread loginThread = new Thread( LoginAsync );
			loginThread.Name = "Launcher.LoginAsync_" + HostServer;
			loginThread.IsBackground = true;
			loginThread.Start();
		}
		
		void LoginAsync() {
			SetStatus( 0, "Signing in.." );
			try {
				Session.Login( Username.Text, Password.Text );
			} catch( WebException ex ) {
				Session.Username = null;
				LoginEnd( false );
				DisplayWebException( ex, "sign in", SetStatus );
				return;
			} catch( InvalidOperationException ex ) {
				Session.Username = null;
				LoginEnd( false );
				string text = "Failed to sign in: " + Environment.NewLine + ex.Message;
				SetStatus( 0, text );
				return;
			}
			
			SetStatus( 50, "Retrieving public servers list.." );
			try {
				Servers = Session.GetPublicServers();
			} catch( WebException ex ) {
				Servers = new List<ServerListEntry>();
				LoginEnd( false );
				DisplayWebException( ex, "retrieve servers list", SetStatus );
				return;
			}
			SetStatus( 100, "Done" );
			LoginEnd( true );
		}
		
		public void LoginEnd( bool success ) {
			if( form.InvokeRequired ) {
				form.Invoke( (Action<bool>)LoginEnd, success );
			} else {
				GC.Collect();
				if( success ) {
					Tab.TabPages.Add( ServersTab );
					FilterList();
				}
				SignInButton.Enabled = true;
			}
		}

		void SetStatus( int percentage, string text ) {
			if( form.InvokeRequired ) {
				form.Invoke( (Action<int, string>)SetStatus, percentage, text );
			} else {
				Progress.Value = percentage;
				Status.Text = text;
			}
		}
		
		public void ConnectToServer() {
			GameStartData data = null;
			try {
				data = Session.GetConnectInfo( Hash.Text );
			} catch( WebException ex ) {
				DisplayWebException( ex, "retrieve server info",
				                    (p, text) => MessageBox.Show( text ) );
				return;
			}
			MainForm.StartClient( data, true );
		}
		
		public void FilterList() {
			ServersTable.Items.Clear();
			if( Servers.Count == 0 ) return;
			IComparer sorter = ServersTable.ListViewItemSorter;
			ServersTable.ListViewItemSorter = null;
			
			for( int i = 0; i < Servers.Count; i++ ) {
				ServerListEntry entry = Servers[i];
				if( Filter( entry ) ) {
					string[] row = { entry.Name, entry.Players, entry.MaximumPlayers, entry.Uptime, entry.Hash };
					ServersTable.Items.Add( new ListViewItem( row ) );
				}
			}
			if( sorter != null ) {
				ServersTable.ListViewItemSorter = sorter;
				ServersTable.Sort();
			}
			ServersTable.EndUpdate();
		}
		
		public void HashChanged() {
			string hash = Hash.Text;
			ListView.SelectedIndexCollection selected = ServersTable.SelectedIndices;
			if( selected.Count > 0 ) {
				ServersTable.Items[selected[0]].Selected = false;
			}
			
			int count = ServersTable.Items.Count;
			for( int i = 0; i < count; i++ ) {
				ListViewItem entry = ServersTable.Items[i];
				if( hash == entry.SubItems[4].Text ) {
					entry.Selected = true;
					ServersTable.EnsureVisible( i );
					break;
				}
			}
		}
		
		public void ServerTableClick() {
			Point mousePos = ServersTable.PointToClient( MainForm.MousePosition );
			ListViewHitTestInfo hitTest = ServersTable.HitTest( mousePos );
			if( hitTest != null && hitTest.Item != null ) {
				Hash.Text = hitTest.Item.SubItems[4].Text;
			}
		}
		
		void DisplayWebException( WebException ex, string action, Action<int, string> target ) {
			Program.LogException( ex );
			string host = HostServer;
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
				string text = "Failed to " + action + ":" + Environment.NewLine + ex.Status;
				target( 0, text );
			}
		}
	}
}