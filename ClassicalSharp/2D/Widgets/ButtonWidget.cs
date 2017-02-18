// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using System.Drawing;
using OpenTK.Input;
#if ANDROID
using Android.Graphics;
#endif

namespace ClassicalSharp.Gui.Widgets {
	public sealed class ButtonWidget : Widget {
		
		public ButtonWidget(Game game, Font font) : base(game) {
			this.font = font;
		}
		
		public static ButtonWidget Create(Game game, int width, string text, Font font, ClickHandler onClick) {
			ButtonWidget widget = new ButtonWidget(game, font);
			widget.Init();
			widget.MinWidth = width; widget.MinHeight = 40;
			widget.SetText(text);
			widget.OnClick = onClick;
			return widget;
		}
		
		public ButtonWidget SetLocation(Anchor horAnchor, Anchor verAnchor, int xOffset, int yOffset) {
			HorizontalAnchor = horAnchor; VerticalAnchor = verAnchor;
			XOffset = xOffset; YOffset = yOffset;
			CalculatePosition();
			return this;
		}
		
		Texture texture;
		public int MinWidth, MinHeight;
		int defaultHeight;
		internal Font font;
		
		public override void Init() {
			DrawTextArgs args = new DrawTextArgs("I", font, true);
			defaultHeight = game.Drawer2D.MeasureChatSize(ref args).Height;
			Height = defaultHeight;
		}
		
		const float uWidth = 200/256f;
		static Texture shadowTex = new Texture(0, 0, 0, 0, 0,
		                                       new TextureRec(0,   66/256f, uWidth, 20/256f));
		static Texture selectedTex = new Texture(0, 0, 0, 0, 0,
		                                         new TextureRec(0, 86/256f, uWidth, 20/256f));
		static Texture disabledTex = new Texture(0, 0, 0, 0, 0,
		                                         new TextureRec(0, 46/256f, uWidth, 20/256f));
		public string Text;
		public void SetText(string text) {
			gfx.DeleteTexture(ref texture);
			Text = text;
			if (String.IsNullOrEmpty(text)) {
				texture = default(Texture);
				Width = 0; Height = defaultHeight;
			} else {
				DrawTextArgs args = new DrawTextArgs(text, font, true);
				texture = game.Drawer2D.MakeChatTextTexture(ref args, 0, 0);
				Width = Math.Max(texture.Width, MinWidth);
				Height = Math.Max(texture.Height, MinHeight);
				
				CalculatePosition();
				texture.X1 = X + (Width / 2 - texture.Width / 2);
				texture.Y1 = Y + (Height / 2 - texture.Height / 2);
			}
		}
		
		static FastColour normCol = new FastColour(224, 224, 224),
		activeCol = new FastColour(255, 255, 160),
		disabledCol = new FastColour(160, 160, 160);
		
		public override void Render(double delta) {
			if (!texture.IsValid) return;
			Texture back = Active ? selectedTex : shadowTex;
			if (Disabled) back = disabledTex;
			
			back.ID = game.UseClassicGui ? game.Gui.GuiClassicTex : game.Gui.GuiTex;
			back.X1 = X; back.Y1 = Y;
			back.Width = (short)Width; back.Height = (short)Height;			
			
			if (Width == 400) {
				// Button can be drawn normally
				back.U1 = 0; back.U2 = uWidth;
				back.Render(gfx);
			} else {				
				// Split button down the middle
				float scale = (Width / 400f) * 0.5f;
				gfx.BindTexture(back.ID); // avoid bind twice
				
				back.Width = (short)(Width / 2); 
				back.U1 = 0; back.U2 = uWidth * scale;
				gfx.Draw2DTexture(ref back, FastColour.White);
				
				back.X1 += (short)(Width / 2); 
				back.U1 = uWidth - uWidth * scale; back.U2 = uWidth;
				gfx.Draw2DTexture(ref back, FastColour.White);
			}
			
			FastColour col = Disabled ? disabledCol : (Active ? activeCol : normCol);
			texture.Render(gfx, col);
		}
		
		public override void Dispose() {
			gfx.DeleteTexture(ref texture);
		}
		
		public override void CalculatePosition() {
			int oldX = X, oldY = Y;
			base.CalculatePosition();
			
			texture.X1 += X - oldX;
			texture.Y1 += Y - oldY;
		}
		
		public Func<Game, string> GetValue;
		public Action<Game, string> SetValue;
	}
}