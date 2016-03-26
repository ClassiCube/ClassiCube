// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.Drawing;

namespace ClassicalSharp {
	
	/// <summary> Draws text to the screen, uses the given font when the user has specified 'use arial' in options,
	/// otherwise draws text using the bitmapped font in default.png </summary>
	public sealed class ChatTextWidget : TextWidget {
		
		public ChatTextWidget( Game game, Font font ) : base( game, font ) {
		}
		
		public static new ChatTextWidget Create( Game game, int x, int y, string text, Anchor horizontal, Anchor vertical, Font font ) {
			ChatTextWidget widget = new ChatTextWidget( game, font );
			widget.Init();
			widget.HorizontalAnchor = horizontal; widget.VerticalAnchor = vertical;
			widget.XOffset = x; widget.YOffset = y;
			widget.SetText( text );
			return widget;
		}
		
		public override void Init() {
			DrawTextArgs args = new DrawTextArgs( "I", font, true );
			defaultHeight = game.Drawer2D.MeasureChatSize( ref args ).Height;
			Height = defaultHeight;
		}
		
		public override void SetText( string text ) {
			graphicsApi.DeleteTexture( ref texture );
			if( String.IsNullOrEmpty( text ) ) {
				texture = new Texture();
				Height = defaultHeight;
			} else {
				DrawTextArgs args = new DrawTextArgs( text, font, true );
				texture = game.Drawer2D.MakeChatTextTexture( ref args, 0, 0 );

				X = texture.X1 = CalcOffset( game.Width, texture.Width, XOffset, HorizontalAnchor );
				Y = texture.Y1 = CalcOffset( game.Height, texture.Height, YOffset, VerticalAnchor );
				Height = texture.Height;
			}
			Width = texture.Width;
		}
	}
}