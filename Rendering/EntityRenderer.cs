using System;
using System.Collections.Generic;
using System.Drawing;
using ClassicalSharp.GraphicsAPI;
using OpenTK;

namespace ClassicalSharp.Renderers {
	
	public class EntityRenderer {
		
		public Game Window;
		public IGraphicsApi Graphics;
		public Player Player;
		
		public EntityRenderer( Player player, Game window ) {
			Player = player;
			Window = window;
			Graphics = window.Graphics;
		}
		
		public virtual void Dispose() {
		}
		
		public virtual void Render( double deltaTime ) {
			Player.Model.RenderModel( Player, this );
		}
	}
	
	public class PlayerRenderer : EntityRenderer {
		
		Texture nameTexture;
		float nameWidth, nameHeight;
		public int TextureId = -1;
		int nameTextureId = -1;
		
		public PlayerRenderer( Player player, Game window ) : base( player, window ) {
			List<DrawTextArgs> parts = Utils.SplitText( Graphics, player.DisplayName, true );
			Size size = Utils2D.MeasureSize( Utils.StripColours( player.DisplayName ), "Arial", 14, true );
			nameTexture = Utils2D.MakeTextTexture( parts, "Arial", 14, size, 0, 0 );
			nameWidth = size.Width;
			nameHeight = size.Height;
			nameTextureId = nameTexture.ID;
		}
		
		public override void Dispose() {
			Graphics.DeleteTexture( TextureId );
			Graphics.DeleteTexture( nameTextureId );
		}
		
		public override void Render( double deltaTime ) {
			base.Render( deltaTime );
			DrawName();
		}
		
		const float nameScale = 50f;
		private void DrawName() {
			Graphics.PushMatrix();
			const float nameYOffset = 2.1375f;
			Vector3 pos = Player.Position;
			Graphics.Translate( pos.X, pos.Y + nameYOffset, pos.Z );
			// Do this to always have names facing the player
			float yaw = Window.LocalPlayer.YawDegrees;
			Graphics.RotateY( 0f - yaw );
			// NOTE: Do this instead with network player's yaw to have names rotate with them instead.
			//Graphics.RotateY( 180f - yaw );
			Graphics.Scale( 1 / nameScale, -1 / nameScale, 1 / nameScale ); // -y to flip text
			Graphics.Translate( -nameWidth / 2f, -nameHeight, 0f );
			
			nameTexture.Render( Graphics );
			
			Graphics.PopMatrix();
			Graphics.Texturing = false;
			Graphics.AlphaTest = false;
		}
	}
}