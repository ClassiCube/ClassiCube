using System;
using System.Drawing;
using ClassicalSharp.GraphicsAPI;

namespace ClassicalSharp {
	
	public class FpsScreen : Screen {
		
		readonly Font font, posFont;
		StringBuffer text;
		
		public FpsScreen( Game game ) : base( game ) {
			font = new Font( "Arial", 13 );
			posFont = new Font( "Arial", 12, FontStyle.Italic );
			text = new StringBuffer( 96 );
		}
		
		TextWidget fpsTextWidget, hackStatesWidget;
		Texture posTexture;
		public override void Render( double delta ) {
			UpdateFPS( delta );
			if( game.HideGui || !game.ShowFPS ) return;
			
			graphicsApi.Texturing = true;
			fpsTextWidget.Render( delta );
			if( game.activeScreen is NormalScreen ) {
				UpdateHackState( false );
				DrawPosition();
				hackStatesWidget.Render( delta );
			}
			graphicsApi.Texturing = false;
		}
		
		double accumulator, maxDelta;
		int fpsCount;
		void UpdateFPS( double delta ) {
			fpsCount++;
			maxDelta = Math.Max( maxDelta, delta );
			accumulator += delta;

			if( accumulator >= 1 ) {
				int index = 0;
				text.Clear()
					.Append( ref index, "FPS: " ).AppendNum( ref index, (int)(fpsCount / accumulator) )
					.Append( ref index, " (min " ).AppendNum( ref index, (int)(1f / maxDelta) )
					.Append( ref index, "), chunks/s: " ).AppendNum( ref index, game.ChunkUpdates )
					.Append( ref index, ", vertices: " ).AppendNum( ref index, game.Vertices );
				
				string textString = text.GetString();
				fpsTextWidget.SetText( textString );
				maxDelta = 0;
				accumulator = 0;
				fpsCount = 0;
				game.ChunkUpdates = 0;
			}
		}
		
		public override void Init() {
			fpsTextWidget = new TextWidget( game, font );
			fpsTextWidget.XOffset = 2;
			fpsTextWidget.YOffset = 2;
			fpsTextWidget.Init();
			fpsTextWidget.SetText( "FPS: no data yet" );
			MakePosTextWidget();
			
			hackStatesWidget = new TextWidget( game, posFont );
			hackStatesWidget.XOffset = 2;
			hackStatesWidget.YOffset = fpsTextWidget.Height + posHeight + 2;
			hackStatesWidget.Init();
			UpdateHackState( true );
		}
		
		public override void Dispose() {
			font.Dispose();
			posFont.Dispose();
			fpsTextWidget.Dispose();
			graphicsApi.DeleteTexture( ref posTexture );
		}
		
		public override void OnResize( int oldWidth, int oldHeight, int width, int height ) {
		}
		
		void DrawPosition() {
			int index = 0;
			TextureRec xy = new TextureRec( 2, posTexture.Y1, baseWidth, posTexture.Height );
			TextureRec uv = new TextureRec( 0, 0, posTexture.U2, posTexture.V2 );
			IGraphicsApi.Make2DQuad( xy, uv, game.ModelCache.vertices, ref index );
			
			Vector3I pos = Vector3I.Floor( game.LocalPlayer.Position );
			curX = baseWidth + 2;
			AddChar( 13, ref index );
			AddInt( pos.X, ref index, true );
			AddInt( pos.Y, ref index, true );
			AddInt( pos.Z, ref index, false );
			AddChar( 14, ref index );
			
			graphicsApi.BindTexture( posTexture.ID );
			graphicsApi.DrawDynamicIndexedVb( DrawMode.Triangles,
			                                 game.ModelCache.vb, game.ModelCache.vertices, index, index * 6 / 4 );
		}
		
		bool speeding, noclip, fly;
		void UpdateHackState( bool force ) {
			LocalPlayer p = game.LocalPlayer;
			if( force || p.speeding != speeding || p.noClip != noclip || p.flying != fly ) {
				speeding = p.speeding; noclip = p.noClip; fly = p.flying;
				int index = 0;
				text.Clear();
				
				if( fly ) text.Append( ref index, "Fly ON   " );
				if( speeding ) text.Append( ref index, "Speed ON   " );
				if( noclip ) text.Append( ref index, "Noclip ON   " );
				hackStatesWidget.SetText( text.GetString() );
			}
		}

		const string possibleChars = "0123456789-, ()";
		int[] widths = new int[possibleChars.Length];
		int baseWidth, curX, posHeight;
		float texWidth;
		void MakePosTextWidget() {
			DrawTextArgs args = new DrawTextArgs( "", posFont, true );
			for( int i = 0; i < possibleChars.Length; i++ ) {
				args.Text = new String( possibleChars[i], 1 );
				widths[i] = game.Drawer2D.MeasureSize( ref args ).Width;
			}
			
			using( IDrawer2D drawer = game.Drawer2D ) {
				args.Text = "Feet pos: ";
				Size size = game.Drawer2D.MeasureSize( ref args );
				baseWidth = size.Width;
				size.Width += 16 * possibleChars.Length;
				posHeight = size.Height;
				
				using( Bitmap bmp = IDrawer2D.CreatePow2Bitmap( size ) ) {
					drawer.SetBitmap( bmp );
					drawer.DrawText( ref args, 0, 0 );
					
					for( int i = 0; i < possibleChars.Length; i++ ) {
						args.Text = new String( possibleChars[i], 1 );
						drawer.DrawText( ref args, baseWidth + 16 * i, 0 );
					}
					
					int y = fpsTextWidget.Height + 2;
					posTexture = drawer.Make2DTexture( bmp, size, 0, y );
					posTexture.U2 = (float)baseWidth / bmp.Width;
					posTexture.Width = baseWidth;
					texWidth = bmp.Width;
				}
			}
		}
		
		
		void AddChar( int charIndex, ref int index ) {
			int width = widths[charIndex];
			TextureRec xy = new TextureRec( curX, posTexture.Y1, width, posTexture.Height );
			TextureRec uv = new TextureRec( (baseWidth + charIndex * 16) / texWidth, 0, width / texWidth, posTexture.V2 );
			
			curX += width;
			IGraphicsApi.Make2DQuad( xy, uv, game.ModelCache.vertices, ref index );
		}
		
		void AddInt( int value, ref int index, bool more ) {
			if( value < 0 )
				AddChar( 10, ref index );
			
			int count = 0;
			value = Reverse( Math.Abs( value ), out count );
			for( int i = 0; i < count; i++ ) {
				AddChar( value % 10, ref index ); value /= 10;
			}
			
			if( more )
				AddChar( 11, ref index );
		}
		
		static int Reverse( int value, out int count ) {
			int orig = value, reversed = 0;
			count = 1; value /= 10;
			while( value > 0 ) {
				count++; value /= 10;
			}
			
			for( int i = 0; i < count; i++ ) {
				reversed *= 10;
				reversed += orig % 10;
				orig /= 10;
			}
			return reversed;
		}
	}
}
