// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using ClassicalSharp.Events;
using System;

namespace ClassicalSharp.Renderers {

	/// <summary> Minimialistic environment renderer - only sets the clear colour to be sky colour.
	/// (no fog, clouds, or proper overhead sky) </summary>
	public class MinimalEnvRenderer : EnvRenderer {
		
		public override void Init( Game game ) {
			base.Init( game );
			graphics.Fog = false;
			graphics.ClearColour( map.Env.SkyCol );
		}
		
		public override void Render( double deltaTime ) {
			graphics.ClearColour( map.Env.SkyCol );
		}
		
		public override void OnNewMap( Game game ) { }
		
		public override void OnNewMapLoaded( Game game ) {
			graphics.ClearColour( map.Env.SkyCol );
		}
		
		protected override void EnvVariableChanged( object sender, EnvVarEventArgs e ) {
			if( e.Var == EnvVar.SkyColour ) {
				graphics.ClearColour( map.Env.SkyCol );
			}
		}
	}
}
