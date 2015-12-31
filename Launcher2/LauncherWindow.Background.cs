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
			string texDir = Path.Combine( Program.AppDirectory, "texpacks" );
			string texPack = Options.Get( OptionsKey.DefaultTexturePack ) ?? "default.zip";
			texPack = Path.Combine( texDir, texPack );
			
			if( !File.Exists( texPack ) )
				texPack = Path.Combine( texDir, "default.zip" );
			if( !File.Exists( texPack ) )
				return;
			
			using( Stream fs = new FileStream( texPack, FileMode.Open, FileAccess.Read, FileShare.Read ) ) {
				ZipReader reader = new ZipReader();
				
				reader.ShouldProcessZipEntry = (f) => f == "default.png";
				reader.ProcessZipEntry = ProcessZipEntry;
				reader.Extract( fs );
			}
		}
		
		bool useBitmappedFont;
		void ProcessZipEntry( string filename, byte[] data, ZipEntry entry ) {
			MemoryStream stream = new MemoryStream( data );
			Bitmap bmp = new Bitmap( stream );
			Drawer.SetFontBitmap( bmp );
			useBitmappedFont = !Options.GetBool( OptionsKey.ArialChatFont, false );
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
				
				drawer.UseBitmappedChat = useBitmappedFont;
				DrawTextArgs args = new DrawTextArgs( "&eClassical&fSharp", logoFont, false );
				Size size = drawer.MeasureChatSize( ref args );
				int xStart = Width / 2 - size.Width / 2;
				
				args.Text = "&0Classical&0Sharp";
				drawer.DrawChatText( ref args, xStart + 5, 10 + 5 );
				args.Text = "&eClassical&fSharp";
				drawer.DrawChatText( ref args, xStart, 10 );
				drawer.UseBitmappedChat = false;
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
