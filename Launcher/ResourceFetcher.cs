using System;
using System.IO;
using System.Net;
using System.Windows.Forms;

namespace Launcher {
	
	public class ResourceFetcher {
		
		const string terrainUri = "https://raw.githubusercontent.com/andrewphorn/ClassiCube-Client/master/src/main/resources/terrain.png";
		const string cloudsUri = "https://raw.githubusercontent.com/andrewphorn/ClassiCube-Client/master/src/main/resources/clouds.png";
		const string charUri = "https://raw.githubusercontent.com/andrewphorn/ClassiCube-Client/master/src/main/resources/char.png";
		
		public void Run( MainForm form ) {
			using( WebClient client = new WebClient() ) {
				client.Proxy = null;				
				if( !DownloadData( terrainUri, client, "terrain.png", form ) ) return;
				if( !DownloadData( cloudsUri, client, "clouds.png", form ) ) return;
				if( !DownloadData( charUri, client, "char.png", form ) ) return;
			}
		}
		
		static bool DownloadData( string uri, WebClient client, string output, MainForm form ) {
			if( File.Exists( output ) ) return true;
			form.Text = MainForm.AppName + " - fetching " + output;
			
			byte[] data = null;
			try {
				data = client.DownloadData( uri );
			} catch( WebException ) {
				MessageBox.Show( "Unable to download " + output, "Failed to download resource", MessageBoxButtons.OK, MessageBoxIcon.Error );
				return false;
			}
			
			try {
				File.WriteAllBytes( output, data );
			} catch( IOException ) {
				MessageBox.Show( "Unable to save " + output, "Failed to save resource", MessageBoxButtons.OK, MessageBoxIcon.Error );
				return false;
			}
			return true;
		}
		
		public bool CheckAllResourcesExist() {
			return File.Exists( "terrain.png" ) && File.Exists( "clouds.png" ) && File.Exists( "char.png" );
		}
	}
}
