using System;
using System.Collections.Generic;
using System.Drawing;
using System.Drawing.Drawing2D;
using System.Drawing.Text;

namespace ClassicalSharp {

	public sealed class GdiDrawer2D : IDrawer2D {
		
		StringFormat format;
		Bitmap measuringBmp;
		Graphics measuringGraphics;
		Dictionary<int, SolidBrush> brushCache = new Dictionary<int, SolidBrush>( 16 );
		
		Graphics g;
		
		public GdiDrawer2D( Game game ) {
			graphics = game.Graphics;
			format = StringFormat.GenericTypographic;
			format.FormatFlags |= StringFormatFlags.MeasureTrailingSpaces;
			format.Trimming = StringTrimming.None;
			//format.FormatFlags |= StringFormatFlags.NoWrap;
			//format.FormatFlags |= StringFormatFlags.NoClip;
			measuringBmp = new Bitmap( 1, 1 );
			measuringGraphics = Graphics.FromImage( measuringBmp );
			measuringGraphics.TextRenderingHint = TextRenderingHint.AntiAlias;
		}
		
		public override void SetBitmap( Bitmap bmp ) {
			if( g != null ) {
				Utils.LogWarning( "Previous IDrawer2D.SetBitmap() call was not properly disposed" );
				g.Dispose();
			}
			
			g = Graphics.FromImage( bmp );
			g.TextRenderingHint = TextRenderingHint.AntiAlias;
			g.SmoothingMode = SmoothingMode.HighQuality;
		}
		
		public override void DrawText( Font font, ref DrawTextArgs args, float x, float y ) {
			if( !args.SkipPartsCheck )
				GetTextParts( args.Text );
			
			Brush shadowBrush = GetOrCreateBrush( args.ShadowColour );
			for( int i = 0; i < parts.Count; i++ ) {
				TextPart part = parts[i];
				Brush textBrush = GetOrCreateBrush( part.TextColour );
				if( args.UseShadow )
					g.DrawString( part.Text, font, shadowBrush, x + shadowOffset, y + shadowOffset, format );
				
				g.DrawString( part.Text, font, textBrush, x, y, format );
				x += g.MeasureString( part.Text, font, Int32.MaxValue, format ).Width;
			}
		}
		
		public override void DrawRect( Color colour, int x, int y, int width, int height ) {
			Brush brush = GetOrCreateBrush( colour );
			g.FillRectangle( brush, x, y, width, height );
		}
		
		public override void DrawRectBounds( Color colour, float lineWidth, int x, int y, int width, int height ) {
			using( Pen pen = new Pen( colour, lineWidth ) )
				g.DrawRectangle( pen, x, y, width, height );
		}
		
		public override void DrawRoundedRect( Color colour, float x, float y, float width, float height ) {
			GraphicsPath path = new GraphicsPath();
			float x1 = x, y1 = y, x2 = x + width, y2 = y + height;
			
			const float r = 3, dia = r * 2;
			path.AddArc( x1, y1, dia, dia, 180, 90 );
			path.AddLine( x1 + r, y1, x2 - r, y1 );
			path.AddArc( x2 - dia, y1, dia, dia, 270, 90 );
			path.AddLine( x2, y1 + r, x2, y2 - r );
			path.AddArc( x2 - dia, y2 - dia, dia, dia, 0, 90 );
			path.AddLine( x1 + r, y2, x2 - r, y2 );
			path.AddArc( x1, y2 - dia, dia, dia, 90, 90 );
			path.AddLine( x1, y1 + r, x1, y2 - r );
			path.CloseAllFigures();
			
			using( Brush brush = new SolidBrush( colour ) )
				g.FillPath( brush, path );
			path.Dispose();
		}
		
		public override void Dispose() {
			g.Dispose();
			g = null;
		}		
		
		public override Size MeasureSize( string text, Font font, bool shadow ) {
			GetTextParts( text );
			SizeF total = SizeF.Empty;
			for( int i = 0; i < parts.Count; i++ ) {
				SizeF size = measuringGraphics.MeasureString( parts[i].Text, font, Int32.MaxValue, format );
				total.Height = Math.Max( total.Height, size.Height );
				total.Width += size.Width;
			}
			
			if( shadow && parts.Count > 0 ) {
				total.Width += shadowOffset;
				total.Height += shadowOffset;
			}
			return Size.Ceiling( total );
		}
		
		public override void DisposeInstance() {
			measuringBmp.Dispose();
			measuringGraphics.Dispose();
			foreach( var pair in brushCache ) {
				pair.Value.Dispose();
			}
		}
		
		SolidBrush GetOrCreateBrush( Color color ) {
			int key = color.ToArgb();
			SolidBrush brush;
			if( brushCache.TryGetValue( key, out brush ) ) {
				return brush;
			}
			brush = new SolidBrush( color );
			brushCache[key] = brush;
			return brush;
		}
	}
}
