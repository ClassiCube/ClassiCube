using System;
using System.Collections.Generic;
using System.Drawing;
using System.Drawing.Drawing2D;
using System.Drawing.Text;
using ClassicalSharp.GraphicsAPI;

namespace ClassicalSharp {

	public sealed class GdiPlusDrawer2D : IDrawer2D {
		
		StringFormat format;
		Bitmap measuringBmp;
		Graphics measuringGraphics;
		Dictionary<int, SolidBrush> brushCache = new Dictionary<int, SolidBrush>( 16 );
		
		Graphics g;
		
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
		
		public override void SetBitmap( Bitmap bmp ) {
			if( g != null ) {
				Utils.LogWarning( "Previous IDrawer2D.SetBitmap() call was not properly disposed" );
				g.Dispose();
			}
			
			g = Graphics.FromImage( bmp );
			g.TextRenderingHint = TextRenderingHint.AntiAlias;
			g.SmoothingMode = SmoothingMode.HighQuality;
		}
		
		public override void DrawText( ref DrawTextArgs args, float x, float y ) {
			if( !args.SkipPartsCheck )
				GetTextParts( args.Text );
			
			Brush shadowBrush = GetOrCreateBrush( Color.Black );
			for( int i = 0; i < parts.Count; i++ ) {
				TextPart part = parts[i];
				Brush textBrush = GetOrCreateBrush( part.TextColour );
				if( args.UseShadow )
					g.DrawString( part.Text, args.Font, shadowBrush, x + Offset, y + Offset, format );
				
				g.DrawString( part.Text, args.Font, textBrush, x, y, format );
				x += g.MeasureString( part.Text, args.Font, Int32.MaxValue, format ).Width;
			}
		}
		
		public override void DrawClippedText( ref DrawTextArgs args, float x, float y, float maxWidth, float maxHeight ) {
			if( !args.SkipPartsCheck )
				GetTextParts( args.Text );
			
			Brush shadowBrush = GetOrCreateBrush( Color.Black );
			format.Trimming = StringTrimming.EllipsisCharacter;
			for( int i = 0; i < parts.Count; i++ ) {
				TextPart part = parts[i];
				Brush textBrush = GetOrCreateBrush( part.TextColour );
				RectangleF rect = new RectangleF( x + Offset, y + Offset, maxWidth, maxHeight );
				if( args.UseShadow )
					g.DrawString( part.Text, args.Font, shadowBrush, rect, format );
				
				rect = new RectangleF( x, y, maxWidth, maxHeight );
				g.DrawString( part.Text, args.Font, textBrush, rect, format );
				x += g.MeasureString( part.Text, args.Font, Int32.MaxValue, format ).Width;
			}
			format.Trimming = StringTrimming.None;
		}
		
		public override void DrawRect( FastColour colour, int x, int y, int width, int height ) {
			Brush brush = GetOrCreateBrush( colour );
			g.FillRectangle( brush, x, y, width, height );
		}
		
		public override void DrawRectBounds( FastColour colour, float lineWidth, int x, int y, int width, int height ) {
			using( Pen pen = new Pen( colour, lineWidth ) ) {
				pen.Alignment = PenAlignment.Inset;
				g.DrawRectangle( pen, x, y, width, height );
			}
		}
		
		public override void DrawRoundedRect( FastColour colour, float radius, float x, float y, float width, float height ) {
			GraphicsPath path = new GraphicsPath();
			float x1 = x, y1 = y, x2 = x + width, y2 = y + height;
			
			float r = radius, dia = radius * 2;
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
		
		public override void Clear( FastColour colour ) {
			g.Clear( colour );
		}
		
		public override void Clear( FastColour colour, int x, int y, int width, int height ) {
			g.SmoothingMode = SmoothingMode.None;
			Brush brush = GetOrCreateBrush( colour );
			g.FillRectangle( brush, x, y, width, height );
			g.SmoothingMode = SmoothingMode.HighQuality;
		}
		
		public override void Dispose() {
			g.Dispose();
			g = null;
		}

		public override Bitmap ConvertTo32Bpp( Bitmap src ) {
			Bitmap bmp = new Bitmap( src.Width, src.Height );
			using( Graphics g = Graphics.FromImage( bmp ) ) {
				g.InterpolationMode = InterpolationMode.NearestNeighbor;
				g.DrawImage( src, 0, 0, src.Width, src.Height );
			}
			return bmp;
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
