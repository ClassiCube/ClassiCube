// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using ClassicalSharp;

namespace Launcher {
	
	public sealed class ResourcesView : IView {
		
		public ResourcesView( LauncherWindow game ) : base( game ) {
			widgets = new LauncherWidget[4];
		}

		public override void Init() {
			// TODO: figure out how to fix this.
		}
		
		public override void DrawAll() {
			using( drawer ) {
				drawer.SetBitmap( game.Framebuffer );
				drawer.Clear( clearCol );
				drawer.Clear( backCol, game.Width / 2 - 190, game.Height / 2 - 70, 190 * 2, 70 * 2 );
			}
			
			RedrawAllButtonBackgrounds();
			using( drawer ) {
				drawer.SetBitmap( game.Framebuffer );
				RedrawAll();
			}
		}
		
		internal void DrawProgressBox( int progress ) {
			progress = (200 * progress) / 100;
			using( drawer ) {
				drawer.SetBitmap( game.Framebuffer );
				drawer.DrawRect( progBack, game.Width / 2 - 100, game.Height / 2 + 10, 200, 4 );
				drawer.DrawRect( progFront, game.Width / 2 - 100, game.Height / 2 + 10, progress, 4 );
			}
		}
		
		internal int lastProgress = int.MinValue;
		static FastColour backCol = new FastColour( 120, 85, 151 );
		static FastColour clearCol = new FastColour( 12, 12, 12 );		
		static FastColour progBack = new FastColour( 220, 220, 220 );
		static FastColour progFront = new FastColour( 0, 220, 0 );
	}
}
