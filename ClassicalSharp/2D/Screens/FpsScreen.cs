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
			fpsTextWidget.Init();
			fpsTextWidget.SetText( "FPS: no data yet" );
		}
		
		public override void Dispose() {
			font.Dispose();
			fpsTextWidget.Dispose();
		}
	}
}
