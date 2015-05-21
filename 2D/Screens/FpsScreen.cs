using System;
using System.Drawing;

namespace ClassicalSharp {
	
	public class FpsScreen : Screen {
		
		readonly Font font;
		UnsafeString text;
		public FpsScreen( Game window ) : base( window ) {
			font = new Font( "Arial", 13 );
			text = new UnsafeString( 96 );
		}
		
		TextWidget fpsTextWidget;
		
		public override void Render( double delta ) {
			UpdateFPS( delta );
			fpsTextWidget.Render( delta );
		}
		
		double accumulator, maxDelta;
		int fpsCount;	
		unsafe void UpdateFPS( double delta ) {
			fpsCount++;
			maxDelta = Math.Max( maxDelta, delta );
			accumulator += delta;
			long vertices = Window.Vertices;

			if( accumulator >= 1 )  {
				fixed( char* ptr = text.value ) {
					char* ptr2 = ptr;
					text.Clear( ptr2 )
						.Append( ref ptr2, "FPS: " ).AppendNum( ref ptr2, (int)( fpsCount / accumulator ) )
						.Append( ref ptr2, " (min " ).AppendNum( ref ptr2, (int)( 1f / maxDelta ) )
						.Append( ref ptr2, "), chunks/s: " ).AppendNum( ref ptr2, Window.ChunkUpdates )
						.Append( ref ptr2, ", vertices: " ).AppendNum( ref ptr2, Window.Vertices );
				}
				string textString = text.value;
				if( Utils2D.needWinXpFix )
					textString = textString.TrimEnd( Utils2D.trimChars );
				
				fpsTextWidget.SetText( textString );
				maxDelta = 0;
				accumulator = 0;
				fpsCount = 0;
				Window.ChunkUpdates = 0;
			}
		}
		
		public override void Init() {
			fpsTextWidget = new TextWidget( Window, font );
			fpsTextWidget.Init();
			fpsTextWidget.SetText( "FPS: no data yet" );
		}
		
		public override void Dispose() {
			font.Dispose();
			fpsTextWidget.Dispose();
		}
	}
}
