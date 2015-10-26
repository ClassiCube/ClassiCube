using System;
using System.Drawing;
using System.Drawing.Text;
using ClassicalSharp.GraphicsAPI;

namespace ClassicalSharp {

	public sealed class GdiPlusDrawerFont : GdiPlusDrawer2D {
		
		StringFormat format;
		Bitmap measuringBmp;
		Graphics measuringGraphics;
		
		public GdiPlusDrawerFont( IGraphicsApi graphics ) : base( graphics ) {
			format = StringFormat.GenericTypographic;
			format.FormatFlags |= StringFormatFlags.MeasureTrailingSpaces;
			format.Trimming = StringTrimming.None;
			//format.FormatFlags |= StringFormatFlags.NoWrap;
			//format.FormatFlags |= StringFormatFlags.NoClip;
			
			measuringBmp = new Bitmap( 1, 1 );
			measuringGraphics = Graphics.FromImage( measuringBmp );
			measuringGraphics.TextRenderingHint = TextRenderingHint.AntiAlias;
		}
		
		public override void DrawText( ref DrawTextArgs args, int x, int y ) {
			if( !args.SkipPartsCheck )
				GetTextParts( args.Text );
			
			Brush shadowBrush = GetOrCreateBrush( Color.Black );
			float textX = x;
			for( int i = 0; i < parts.Count; i++ ) {
				TextPart part = parts[i];
				Brush textBrush = GetOrCreateBrush( part.TextColour );
				if( args.UseShadow )
					g.DrawString( part.Text, args.Font, shadowBrush, textX + Offset, y + Offset, format );
				
				g.DrawString( part.Text, args.Font, textBrush, textX, y, format );
				textX += g.MeasureString( part.Text, args.Font, Int32.MaxValue, format ).Width;
			}
		}
		
		public override void DrawClippedText( ref DrawTextArgs args, int x, int y, float maxWidth, float maxHeight ) {
			if( !args.SkipPartsCheck )
				GetTextParts( args.Text );
			
			Brush shadowBrush = GetOrCreateBrush( Color.Black );
			format.Trimming = StringTrimming.EllipsisCharacter;
			float textX = x;
			
			for( int i = 0; i < parts.Count; i++ ) {
				TextPart part = parts[i];
				Brush textBrush = GetOrCreateBrush( part.TextColour );
				RectangleF rect = new RectangleF( textX + Offset, y + Offset, maxWidth, maxHeight );
				if( args.UseShadow )
					g.DrawString( part.Text, args.Font, shadowBrush, rect, format );
				
				rect = new RectangleF( textX, y, maxWidth, maxHeight );
				g.DrawString( part.Text, args.Font, textBrush, rect, format );
				textX += g.MeasureString( part.Text, args.Font, Int32.MaxValue, format ).Width;
			}
			format.Trimming = StringTrimming.None;
		}
		
		public override Size MeasureSize( ref DrawTextArgs args ) {			
			GetTextParts( args.Text );
			SizeF total = SizeF.Empty;
			for( int i = 0; i < parts.Count; i++ ) {
				SizeF size = measuringGraphics.MeasureString( parts[i].Text, args.Font, Int32.MaxValue, format );
				total.Height = Math.Max( total.Height, size.Height );
				total.Width += size.Width;
			}
			
			if( args.UseShadow && parts.Count > 0 ) {
				total.Width += Offset; total.Height += Offset;
			}
			return Size.Ceiling( total );
		}
		
		public override void DisposeInstance() {
			base.DisposeInstance();
			measuringGraphics.Dispose();
			measuringBmp.Dispose();
		}
	}
}
