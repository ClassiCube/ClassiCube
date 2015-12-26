using System;
using System.Drawing;
using System.IO;
using ClassicalSharp;
using ClassicalSharp.TexturePack;

namespace Launcher2 {

	public sealed partial class LauncherWindow {
		
		internal FastColour clearColour = new FastColour( 0x39, 0x43, 0x51 );
		internal Bitmap buttonsBmp;
		internal FastBitmap fastButtonsBmp;
		
		internal void TryLoadTexturePack() {
			Options.Load();
			buttonsBmp = new Bitmap( "buttons.png" );
			fastButtonsBmp = new FastBitmap( buttonsBmp, true );
			string texPack = Options.Get( OptionsKey.DefaultTexturePack ) ?? "default.zip";
			
			if( !File.Exists( texPack ) )
				texPack = "default.zip";
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
			if( Framebuffer != null )
				Framebuffer.Dispose();
			
			Framebuffer = new Bitmap( Width, Height );
			using( IDrawer2D drawer = Drawer ) {
				drawer.SetBitmap( Framebuffer );
				ClearArea( 0, 0, Width, Height );
				
				DrawTextArgs args1 = new DrawTextArgs( "&eClassical", logoItalicFont, true );
				Size size1 = drawer.MeasureChatSize( ref args1 );
				DrawTextArgs args2 = new DrawTextArgs( "&fSharp", logoFont, true );
				Size size2 = drawer.MeasureChatSize( ref args2 );
				
				int adjust = Drawer.UseBitmappedChat ? -8 : 2;
				int xStart = Width / 2 - (size1.Width + size2.Width) / 2;
				drawer.DrawChatText( ref args1, xStart, 20 + (size2.Height - size1.Height - 1) );
				drawer.DrawChatText( ref args2, xStart + size1.Width + adjust, 20 );
			}
			Dirty = true;
		}
		
		public void ClearArea( int x, int y, int width, int height ) {
			//FastColour a = new FastColour( 0x39, 0x43, 0x51 );
			//FastColour b = FastColour.Black;
			//FastColour a = FastColour.White, b = FastColour.Black;
			//using( FastBitmap dst = new FastBitmap( Framebuffer, true ) )
			//	Drawer2DExt.DrawGradient( dst, new Rectangle( x, y, width, height ), a, b );
			
			//FastColour col = new FastColour( 169, 143, 192 );
			FastColour col = new FastColour( 132, 111, 140 );
			using( FastBitmap dst = new FastBitmap( Framebuffer, true ) )
				Drawer2DExt.DrawNoise( dst, new Rectangle( x, y, width, height ), col );
		}
	}
}
