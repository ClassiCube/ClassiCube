// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.Drawing;
using ClassicalSharp.GraphicsAPI;
using ClassicalSharp.Gui.Screens;
using OpenTK.Input;

namespace ClassicalSharp.Gui.Widgets {
	public sealed class BlockHotbarWidget : Widget {
		
		public BlockHotbarWidget( Game game ) : base( game ) {
			HorizontalAnchor = Anchor.Centre;
			VerticalAnchor = Anchor.BottomOrRight;
			hotbarCount = game.Inventory.Hotbar.Length;
		}
		
		int hotbarCount;
		Texture selTex, backTex;
		float barHeight, selBlockSize, elemSize;
		float barXOffset, borderSize;
		IsometricBlockDrawer drawer = new IsometricBlockDrawer();
		
		public override void Init() {
			float scale = 2 * game.GuiHotbarScale;
			selBlockSize = (float)Math.Ceiling( 24 * scale );
			barHeight = (int)(22 * scale);		
			Width = (int)(182 * scale);
			Height = (int)barHeight;
			
			elemSize = 16 * scale;
			barXOffset = 3.1f * scale;
			borderSize = 4 * scale;
			X = game.Width / 2 - Width / 2;
			Y = game.Height - Height;
			
			MakeBackgroundTexture();
			MakeSelectionTexture();
		}
		
		public override void Render( double delta ) {
			RenderHotbar();
			Model.ModelCache cache = game.ModelCache;
			drawer.BeginBatch( game, cache.vertices, cache.vb );
			
			for( int i = 0; i < hotbarCount; i++ ) {
				byte block = (byte)game.Inventory.Hotbar[i];
				int x = (int)(X + barXOffset + (elemSize + borderSize) * i + elemSize / 2);
				int y = (int)(game.Height - barHeight / 2);
				
				float scale = (elemSize * 13.5f/16f) / 2f;
				drawer.DrawBatch( block, scale, x, y );
			}
			drawer.EndBatch();
		}
		
		void RenderHotbar() {
			int texId = game.UseClassicGui ? game.Gui.GuiClassicTex : game.Gui.GuiTex;
			backTex.ID = texId;
			backTex.Render( gfx );
			
			int i = game.Inventory.HeldBlockIndex;
			int x = (int)(X + barXOffset + (elemSize + borderSize) * i + elemSize / 2);
			
			selTex.ID = texId;
			selTex.X1 = (int)(x - selBlockSize / 2);
			gfx.Draw2DTexture( ref selTex, FastColour.White );
		}
		
		public override void Dispose() { }
		
		public override void MoveTo( int newX, int newY ) {
			X = newX; Y = newY;
			Recreate();
		}
		
		void MakeBackgroundTexture() {
			TextureRec rec = new TextureRec( 0, 0, 182/256f, 22/256f );
			backTex = new Texture( 0, X, Y, Width, Height, rec );
		}
		
		void MakeSelectionTexture() {
			int hSize = (int)selBlockSize;
			int vSize = (int)Math.Floor( 23 * 2 * game.GuiHotbarScale );
			int y = game.Height - vSize;
			TextureRec rec = new TextureRec( 0, 22/256f, 24/256f, 24/256f );
			selTex = new Texture( 0, 0, y, hSize, vSize, rec );
		}
		
		public override bool HandlesKeyDown( Key key ) {
			if( key >= Key.Number1 && key <= Key.Number9 ) {
				game.Inventory.HeldBlockIndex = (int)key - (int)Key.Number1;
				return true;
			}
			return false;
		}
		
		public override bool HandlesMouseClick( int mouseX, int mouseY, MouseButton button ) {
			if( button != MouseButton.Left || !Bounds.Contains( mouseX, mouseY ) )
			   return false;
			InventoryScreen screen = game.Gui.ActiveScreen as InventoryScreen;
			if( screen == null ) return false;
			
			for( int i = 0; i < hotbarCount; i++ ) {
				int x = (int)(X + (elemSize + borderSize) * i);
				int y = (int)(game.Height - barHeight);
				Rectangle bounds = new Rectangle( x, y, (int)(elemSize + borderSize), (int)barHeight );
					
				if( bounds.Contains( mouseX, mouseY ) ) {
					game.Inventory.HeldBlockIndex = i;
					return true;
				}
			}
			return false;
		}
	}
}