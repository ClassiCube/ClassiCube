using System;
using System.IO;
using System.Net;
using System.Windows.Forms;

namespace Launcher {
	
	public class ResourceFetcher {
		
		const string resUri = "https://raw.githubusercontent.com/andrewphorn/ClassiCube-Client/master/src/main/resources/";
		static readonly string[] coreList = { "char.png", "clouds.png", "terrain.png", "rain.png", "snow.png" };
		static readonly string[] mobsList = { "chicken.png", "creeper.png", "pig.png", "sheep.png",
			"sheep_fur.png", "skeleton.png", "spider.png", "zombie.png" };
		static int resourcesCount = coreList.Length + mobsList.Length;
		
		public void Run( MainForm form ) {
			using( WebClient client = new WebClient() ) {
				client.Proxy = null;
				int i = 0;
				if( DownloadResources( "", client, form, ref i, coreList ) ) {
					DownloadResources( "mob/", client, form, ref i, mobsList );
				}
			}
		}
		
		static bool DownloadResources( string prefix, WebClient client, MainForm form, ref int i, params string[] resources ) {
			foreach( string resource in resources ) {
				if( !DownloadData( prefix + resource, client, resource, form, ref i ) ) return false;
			}
			return true;
		}
		
		static bool DownloadData( string uri, WebClient client, string output, MainForm form, ref int i ) {
			i++;
			if( File.Exists( output ) ) return true;
			form.Text = MainForm.AppName + " - fetching " + output + "(" + i + "/" + resourcesCount + ")";
			
			try {
				client.DownloadFile( resUri + uri, output );
			} catch( WebException ) {
				MessageBox.Show( "Unable to download or save " + output, "Failed to download or save resource", MessageBoxButtons.OK, MessageBoxIcon.Error );
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
