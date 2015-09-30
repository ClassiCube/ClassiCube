using System;
using ClassicalSharp;
using ClassicalSharp.GraphicsAPI;
using OpenTK;
using System.Drawing;

namespace Launcher2 {

	public partial class Game : GameWindow {
		
		public IGraphicsApi Graphics;
		FpsScreen fpsScreen;
		public IDrawer2D Drawer2D;
		
		protected override void OnLoad( EventArgs e ) {
			#if !USE_DX
			//Graphics = new OpenGLApi();
			#else
			Graphics = new Direct3D9Api( this );
			#endif
			Graphics.SetVSync( this, true );
			Graphics.DepthTest = true;
			Graphics.DepthTestFunc( CompareFunc.LessEqual );
			Graphics.AlphaBlendFunc( BlendFunc.SourceAlpha, BlendFunc.InvSourceAlpha );
			Graphics.AlphaTestFunc( CompareFunc.Greater, 0.5f );
			Title = Utils.AppName;
			Width = width;
			Height = height;
			fpsScreen = new FpsScreen( Graphics );
			fpsScreen.Init();
		}
		int width, height;
		
		protected override void OnRenderFrame( FrameEventArgs e ) {
			Graphics.BeginFrame( this );
			
			Graphics.Clear();		
			Graphics.Mode2D( Width, Height );
			fpsScreen.Render( e.Time );
			if( activeScreen != null ) {
				activeScreen.Render( e.Time );
			}
			Graphics.Mode3D();
			
			Graphics.EndFrame( this );
		}
		
		protected override void OnResize( EventArgs e ) {
			base.OnResize( e );
			Graphics.OnWindowResize( this );
			if( activeScreen != null ) {
				activeScreen.OnResize( width, height, Width, Height );
			}
			width = Width;
			height = Height;
		}
		
		Screen activeScreen;
		public void SetNewScreen( Screen screen ) {
			if( activeScreen != null ) {
				activeScreen.Dispose();
			}
			
			activeScreen = screen;
			if( screen != null ) {
				screen.Init();
			}
		}
		
		public override void Dispose() {
			if( activeScreen != null ) {
				activeScreen.Dispose();
			}
			Graphics.Dispose();
			Drawer2D.DisposeInstance();
			base.Dispose();
		}
	}
	
	public class FpsScreen : Screen {
		
		readonly Font font;
		StringBuffer text;
		public FpsScreen( IGraphicsApi gfx ) : base( gfx ) {
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
