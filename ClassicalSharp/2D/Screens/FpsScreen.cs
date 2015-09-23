using System;
using System.Drawing;

namespace ClassicalSharp {
	
	public class FpsScreen : Screen {
		
		readonly Font font;
		StringBuffer text;
		public FpsScreen( Game game ) : base( game ) {
			font = new Font( "Arial", 13 );
			text = new StringBuffer( 96 );
		}
		
		TextWidget fpsTextWidget;
		
		public override void Render( double delta ) {		
			UpdateFPS( delta );
			if( game.HideGui ) return;
			
			graphicsApi.Texturing = true;
			fpsTextWidget.Render( delta );
			graphicsApi.Texturing = false;
		}
		
		double accumulator, maxDelta;
		int fpsCount;	
		unsafe void UpdateFPS( double delta ) {
			fpsCount++;
			maxDelta = Math.Max( maxDelta, delta );
			accumulator += delta;

			if( accumulator >= 1 )  {
				fixed( char* ptr = text.value ) {
					char* ptr2 = ptr;
					text.Clear( ptr2 )
						.Append( ref ptr2, "FPS: " ).AppendNum( ref ptr2, (int)( fpsCount / accumulator ) )
						.Append( ref ptr2, " (min " ).AppendNum( ref ptr2, (int)( 1f / maxDelta ) )
						.Append( ref ptr2, "), chunks/s: " ).AppendNum( ref ptr2, game.ChunkUpdates )
						.Append( ref ptr2, ", vertices: " ).AppendNum( ref ptr2, game.Vertices );
				}
				
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
			fpsTextWidget.Init();
			fpsTextWidget.SetText( "FPS: no data yet" );
		}
		
		public override void Dispose() {
			font.Dispose();
			fpsTextWidget.Dispose();
		}
	}
}
