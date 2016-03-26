// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;

namespace ClassicalSharp.Renderers {

	/// <summary> Minimialistic environment renderer - only sets the clear colour to be sky colour.
	/// (no fog, clouds, or proper overhead sky) </summary>
	public class MinimalEnvRenderer : EnvRenderer {
		
		public MinimalEnvRenderer( Game game ) {
			this.game = game;
			map = game.Map;
		}
		
		public override void Render( double deltaTime ) {
			graphics.ClearColour( map.SkyCol );
		}
		
		public override void Init() {
			base.Init();
			graphics.Fog = false;
			graphics.ClearColour( map.SkyCol );
		}
		
		public override void OnNewMap( object sender, EventArgs e ) {
		}
		
		public override void OnNewMapLoaded( object sender, EventArgs e ) {
			graphics.ClearColour( map.SkyCol );
		}
		
		protected override void EnvVariableChanged( object sender, EnvVarEventArgs e ) {
			if( e.Var == EnvVar.SkyColour ) {
				graphics.ClearColour( map.SkyCol );
			}
		}
	}
}
