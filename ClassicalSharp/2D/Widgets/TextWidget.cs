// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using System.Drawing;

namespace ClassicalSharp.Gui.Widgets {
	public class TextWidget : Widget {		
		public TextWidget(Game game) : base(game) { }
		
		public static TextWidget Create(Game game, string text, Font font) {
			TextWidget w = new TextWidget(game);
			w.Set(text, font);
			return w;
		}
		
		public TextWidget SetLocation(Anchor horAnchor, Anchor verAnchor, int xOffset, int yOffset) {
			HorizontalAnchor = horAnchor; VerticalAnchor = verAnchor;
			XOffset = xOffset; YOffset = yOffset;
			Reposition();
			return this;
		}
		
		protected Texture texture;
		public override void Init() { }
		
		public bool ReducePadding;
		public PackedCol Col = PackedCol.White;
		public bool IsValid { get { return texture.IsValid; } }
		
		public void Set(string text, Font font) {
			game.Graphics.DeleteTexture(ref texture);
			if (IDrawer2D.EmptyText(text)) {
				texture = new Texture();
				int height = game.Drawer2D.FontHeight(font, true);
				texture.Height = (ushort)height;
			} else {
				DrawTextArgs args = new DrawTextArgs(text, font, true);
				texture = game.Drawer2D.MakeTextTexture(ref args, 0, 0);	
			}
			
			if (ReducePadding) {
				game.Drawer2D.ReducePadding(ref texture, Utils.Floor(font.Size), 4);
			}
			
			Width = texture.Width; Height = texture.Height;
			Reposition();
			texture.X1 = X; texture.Y1 = Y;
		}
		
		public override void Render(double delta) {
			if (texture.IsValid) {
				texture.Render(game.Graphics, Col);
			}
		}
		
		public override void Dispose() {
			game.Graphics.DeleteTexture(ref texture);
		}
		
		public override void Reposition() {
			int oldX = X, oldY = Y;
			base.Reposition();
			
			texture.X1 += X - oldX;
			texture.Y1 += Y - oldY;
		}
	}
}