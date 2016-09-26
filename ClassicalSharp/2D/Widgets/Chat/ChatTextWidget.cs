// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.Drawing;

namespace ClassicalSharp.Gui.Widgets {
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
			int height = game.Drawer2D.MeasureChatSize( ref args ).Height;
			SetHeight( height );
		}
		
		public override void SetText( string text ) {
			api.DeleteTexture( ref texture );
			if( String.IsNullOrEmpty( text ) ) {
				texture = new Texture();
				Width = 0; Height = defaultHeight;
			} else {
				DrawTextArgs args = new DrawTextArgs( text, font, true );
				texture = game.Drawer2D.MakeChatTextTexture( ref args, 0, 0 );
				if( ReducePadding )
					game.Drawer2D.ReducePadding( ref texture, Utils.Floor( font.Size ) );
				Width = texture.Width; Height = texture.Height;

				X = texture.X1 = CalcOffset( game.Width, Width, XOffset, HorizontalAnchor );
				Y = texture.Y1 = CalcOffset( game.Height, Height, YOffset, VerticalAnchor );
			}
		}
	}
}