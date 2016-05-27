// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
#if !ANDROID
using System;
using System.Drawing;
using System.Drawing.Text;
using ClassicalSharp.GraphicsAPI;

namespace ClassicalSharp {

	public sealed partial class GdiPlusDrawer2D {
		
		StringFormat format;
		Bitmap measuringBmp;
		Graphics measuringGraphics;
		
		public GdiPlusDrawer2D( IGraphicsApi graphics ) {
			this.graphics = graphics;
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
			
			float textX = x;
			Brush backBrush = GetOrCreateBrush( FastColour.Black );
			for( int i = 0; i < parts.Count; i++ ) {
				TextPart part = parts[i];
				Brush foreBrush = GetOrCreateBrush( part.Col );
				if( args.UseShadow )
					g.DrawString( part.Text, args.Font, backBrush, textX + Offset, y + Offset, format );
				
				g.DrawString( part.Text, args.Font, foreBrush, textX, y, format );
				textX += g.MeasureString( part.Text, args.Font, Int32.MaxValue, format ).Width;
			}
		}
		
		public override void DrawClippedText( ref DrawTextArgs args, int x, int y, float maxWidth, float maxHeight ) {
			if( !args.SkipPartsCheck )
				GetTextParts( args.Text );
			
			Brush shadowBrush = GetOrCreateBrush( FastColour.Black );
			StringFormatFlags flags = format.FormatFlags;
 			format.FormatFlags |= StringFormatFlags.NoWrap;
			format.Trimming = StringTrimming.EllipsisCharacter;
			float textX = x;
			
			for( int i = 0; i < parts.Count; i++ ) {
				TextPart part = parts[i];
				Brush textBrush = GetOrCreateBrush( part.Col );
				RectangleF rect = new RectangleF( textX + Offset, y + Offset, maxWidth, maxHeight );
				if( args.UseShadow )
					g.DrawString( part.Text, args.Font, shadowBrush, rect, format );
				
				rect = new RectangleF( textX, y, maxWidth, maxHeight );
				g.DrawString( part.Text, args.Font, textBrush, rect, format );
				textX += g.MeasureString( part.Text, args.Font, Int32.MaxValue, format ).Width;
			}
			format.Trimming = StringTrimming.None;
			format.FormatFlags = flags;
		}
		
		FastBitmap bitmapWrapper = new FastBitmap();
		public override void DrawBitmappedText( ref DrawTextArgs args, int x, int y ) {
			using( bitmapWrapper ) {
				bitmapWrapper.SetData( curBmp, true, false );
				DrawBitmapTextImpl( bitmapWrapper, ref args, x, y );
			}
		}
		
		public override Size MeasureSize( ref DrawTextArgs args ) {
			GetTextParts( args.Text );
			if( parts.Count == 0 ) return Size.Empty;
			
			SizeF total = SizeF.Empty;
			for( int i = 0; i < parts.Count; i++ ) {
				SizeF size = measuringGraphics.MeasureString( parts[i].Text, args.Font, Int32.MaxValue, format );
				total.Height = Math.Max( total.Height, size.Height );
				total.Width += size.Width;
			}
			
			if( args.UseShadow ) {
				total.Width += Offset; total.Height += Offset;
			}
			return Size.Ceiling( total );
		}
		
		public override Size MeasureBitmappedSize( ref DrawTextArgs args ) {
			return MeasureBitmappedSizeImpl( ref args );
		}
		
		void DisposeText() {
			measuringGraphics.Dispose();
			measuringBmp.Dispose();
		}
	}
}
#endif