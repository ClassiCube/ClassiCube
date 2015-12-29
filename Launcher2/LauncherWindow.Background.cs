using System;
using System.Drawing;
using System.IO;
using ClassicalSharp;
using ClassicalSharp.TexturePack;

namespace Launcher2 {

	public sealed partial class LauncherWindow {

		internal void TryLoadTexturePack() {
			Options.Load();
			LauncherSkin.LoadFromOptions();
			string texPack = Options.Get( OptionsKey.DefaultTexturePack ) ?? "default.zip";
			texPack = Path.Combine( Program.AppDirectory, texPack );
			
			if( !File.Exists( texPack ) )
				texPack = Path.Combine( Program.AppDirectory, "default.zip" );
			if( !File.Exists( texPack ) )
				return;
			
			using( Stream fs = new FileStream( texPack, FileMode.Open, FileAccess.Read, FileShare.Read ) ) {
				ZipReader reader = new ZipReader();
				
				reader.ShouldProcessZipEntry = (f) => f == "default.png";
				reader.ProcessZipEntry = ProcessZipEntry;
				reader.Extract( fs );
			}
		}
		
		void ProcessZipEntry( string filename, byte[] data, ZipEntry entry ) {
			MemoryStream stream = new MemoryStream( data );
			Bitmap bmp = new Bitmap( stream );
			Drawer.SetFontBitmap( bmp );
			Drawer.UseBitmappedChat = !Options.GetBool( OptionsKey.ArialChatFont, false );
		}
		
		public void MakeBackground() {
			if( Framebuffer == null || (Framebuffer.Width != Width || Framebuffer.Height != Height) ) {
				if( Framebuffer != null ) 
					Framebuffer.Dispose();
				Framebuffer = new Bitmap( Width, Height );
			}			
			using( IDrawer2D drawer = Drawer ) {
				drawer.SetBitmap( Framebuffer );
				ClearArea( 0, 0, Width, Height );
				
				DrawTextArgs args = new DrawTextArgs( "&eClassical&fSharp", logoFont, false );
				Size size = drawer.MeasureChatSize( ref args );
				int xStart = Width / 2 - size.Width / 2;
				
				args.Text = "&0Classical&0Sharp";
				drawer.DrawChatText( ref args, xStart + 4, 20 + 4 );
				args.Text = "&eClassical&fSharp";
				drawer.DrawChatText( ref args, xStart, 20 );
			}
			Dirty = true;
		}
		
		public void ClearArea( int x, int y, int width, int height ) {
			FastColour col = LauncherSkin.BackgroundCol;
			using( FastBitmap dst = new FastBitmap( Framebuffer, true ) )
				Drawer2DExt.DrawNoise( dst, new Rectangle( x, y, width, height ), col );
		}
	}
}
