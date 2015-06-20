using System;
using System.Drawing;
using ClassicalSharp;
using ClassicalSharp.Renderers;

namespace DefaultPlugin {

	/// <summary> Minimialistic environment renderer - only sets the clear colour to be sky colour.
	/// (no fog, clouds, or proper overhead sky) </summary>
	public class MinimalEnvRenderer : EnvRenderer {
		
		public MinimalEnvRenderer( Game window ) {
			Window = window;
			Map = Window.Map;
		}
		
		public override void Render( double deltaTime ) {
			Graphics.ClearColour( Map.SkyCol );
		}
		
		public override void Init() {
			base.Init();
			Graphics.ClearColour( Map.SkyCol );
		}
		
		public override void OnNewMap( object sender, EventArgs e ) {
		}
		
		public override void OnNewMapLoaded( object sender, EventArgs e ) {
			Graphics.ClearColour( Map.SkyCol );
		}
		
		protected override void CloudsColourChanged() {
		}
		
		protected override void FogColourChanged() {
		}
		
		protected override void SkyColourChanged() {
		}
	}
}
