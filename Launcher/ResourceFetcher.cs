using System;
using System.IO;
using System.Net;
using System.Windows.Forms;

namespace Launcher {
	
	public class ResourceFetcher {
		
		const string resUri = "https://raw.githubusercontent.com/andrewphorn/ClassiCube-Client/master/src/main/resources/";
		static readonly string[] coreList = { "char.png", "clouds.png", "terrain.png" };
		static readonly string[] mobsList = { "chicken.png", "creeper.png", "pig.png", "sheep.png",
			"sheep_fur.png", "skeleton.png", "spider.png", "zombie.png" };
		
		public void Run( MainForm form ) {
			using( WebClient client = new WebClient() ) {
				client.Proxy = null;
				if( DownloadResources( "", client, form, coreList ) ) {
					DownloadResources( "mob/", client, form, mobsList );
				}
			}
		}
		
		static bool DownloadResources( string prefix, WebClient client, MainForm form, params string[] resources ) {
			foreach( string resource in resources ) {
				if( !DownloadData( prefix + resource, client, resource, form ) ) return false;
			}
			return true;
		}
		
		static bool DownloadData( string uri, WebClient client, string output, MainForm form ) {
			if( File.Exists( output ) ) return true;
			form.Text = MainForm.AppName + " - fetching " + output;
			
			try {
				byte[] data = client.DownloadData( resUri + uri );
				try {
					File.WriteAllBytes( output, data );
				} catch( IOException ) {
					MessageBox.Show( "Unable to save " + output, "Failed to save resource", MessageBoxButtons.OK, MessageBoxIcon.Error );
					return false;
				}
			} catch( WebException ) {
				MessageBox.Show( "Unable to download " + output, "Failed to download resource", MessageBoxButtons.OK, MessageBoxIcon.Error );
				return false;
			}
			return true;
		}

		public bool CheckAllResourcesExist() {
			return CheckResourcesExist( coreList ) && CheckResourcesExist( mobsList );
		}

		static bool CheckResourcesExist( params string[] resources ) {
			foreach( string file in resources ) {
				if( !File.Exists( file ) )
					return false;
			}
			return true;
		}
	}
}
