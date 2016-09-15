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
			if( map.IsNotLoaded ) return;
			FastColour fogCol = FastColour.White;
			float fogDensity = 0;
			byte block = BlockOn( out fogDensity, out fogCol );
			graphics.ClearColour( fogCol );
		}
		
		public override void OnNewMap( Game game ) { }		
		public override void OnNewMapLoaded( Game game ) { }		
		protected override void EnvVariableChanged( object sender, EnvVarEventArgs e ) { }
	}
}
