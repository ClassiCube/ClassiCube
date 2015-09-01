using System;
using ClassicalSharp.GraphicsAPI;

namespace ClassicalSharp.Renderers {

	public abstract class EnvRenderer : IDisposable {
		
		protected Map map;
		protected Game game;	
		protected IGraphicsApi graphics;
		
		public virtual void Init() {
			graphics = game.Graphics;
			game.OnNewMap += OnNewMap;
			game.OnNewMapLoaded += OnNewMapLoaded;
			game.EnvVariableChanged += EnvVariableChanged;
		}		
		
		public virtual void OnNewMap( object sender, EventArgs e ) {
		}
		
		public virtual void OnNewMapLoaded( object sender, EventArgs e ) {
		}
		
		public virtual void Dispose() {
			game.OnNewMap -= OnNewMap;
			game.OnNewMapLoaded -= OnNewMapLoaded;
			game.EnvVariableChanged -= EnvVariableChanged;
		}
		
		public abstract void Render( double deltaTime );
		
		void EnvVariableChanged( object sender, EnvVariableEventArgs e ) {
			if( e.Var == EnvVariable.SkyColour ) {
				SkyColourChanged();
			} else if( e.Var == EnvVariable.FogColour ) {
				FogColourChanged();
			} else if( e.Var == EnvVariable.CloudsColour ) {
				CloudsColourChanged();
			}
		}
		
		protected abstract void SkyColourChanged();
		
		protected abstract void FogColourChanged();
		
		protected abstract void CloudsColourChanged();
	}
}
