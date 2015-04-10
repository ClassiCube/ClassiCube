using System;
using OpenTK;

namespace SoftwareRasterizer.Engine {
	
	public unsafe partial class SoftwareEngine {
		
		public void Filled_DrawIndexedTriangles( Vector3[] vertices, ushort[] indices ) {
			for( int i = 0; i < indices.Length; i += 3 ) {
				Vector3 vertex1 = vertices[indices[i + 0]];
				Vector3 vertex2 = vertices[indices[i + 1]];
				Vector3 vertex3 = vertices[indices[i + 2]];

				float x1 = 0, y1 = 0, x2 = 0, y2 = 0, x3 = 0, y3 = 0;
				Project( ref vertex1, out x1, out y1 );
				Project( ref vertex2, out x2, out y2 );
				Project( ref vertex3, out x3, out y3 );
				byte col = (byte)( 255 * ( 0.25f + ( ( i / 3 ) % indices.Length ) * 0.75f / indices.Length ) );
				FastColour col2 = new FastColour( col, col, col );
				DrawTriangle( new Vector3( x1, y1, 0 ), new Vector3( x2, y2, 0 ), new Vector3( x3, y3, 0 ), ref col2 );
			}
		}
		
		float Clamp( float value ) {
			if( value < 0 ) return 0;
			if( value > 1 ) return 1;
			return value;
		}

		float lerp( float min, float max, float gradient ) {
			return min + ( max - min ) * Clamp( gradient );
		}

		void ProcessScanLine( int y, Vector3 pa, Vector3 pb, Vector3 pc, Vector3 pd, ref FastColour col ) {
			float gradient1 = pa.Y != pb.Y ? (y - pa.Y) / (pb.Y - pa.Y) : 1;
			float gradient2 = pc.Y != pd.Y ? (y - pc.Y) / (pd.Y - pc.Y) : 1;
			
			int sx = (int)lerp( pa.X, pb.X, gradient1 );
			int ex = (int)lerp( pc.X, pd.X, gradient2 );

			for( int x = sx; x < ex; x++ ) {
				SetPixelClipped( ref x, ref y, ref col );
			}
		}

		void Swap( ref Vector3 a, ref Vector3 b ) {
			Vector3 c = a;
			a = b;
			b = c;
		}
		
		void DrawTriangle( Vector3 p1, Vector3 p2, Vector3 p3, ref FastColour col ) {
			if( p1.Y > p2.Y ) Swap( ref p1, ref p2 );
			if( p2.Y > p3.Y ) Swap( ref p2, ref p3 );
			if( p1.Y > p2.Y ) Swap( ref p1, ref p2 );

			float dP1P2, dP1P3;

			// http://en.wikipedia.org/wiki/Slope
			// Computing inverse slopes
			if( p2.Y - p1.Y > 0 )
				dP1P2 = ( p2.X - p1.X ) / ( p2.Y - p1.Y );
			else
				dP1P2 = 0;

			if( p3.Y - p1.Y > 0 )
				dP1P3 = ( p3.X - p1.X ) / ( p3.Y - p1.Y );
			else
				dP1P3 = 0;

			if( dP1P2 > dP1P3 ) {
				for( int y = (int)p1.Y; y <= (int)p3.Y; y++ ) {
					if( y < p2.Y ) {
						ProcessScanLine( y, p1, p3, p1, p2, ref col );
					} else {
						ProcessScanLine( y, p1, p3, p2, p3, ref col );
					}
				}
			} else {
				for( int y = (int)p1.Y; y <= (int)p3.Y; y++ ) {
					if( y < p2.Y ) {
						ProcessScanLine( y, p1, p2, p1, p3, ref col );
					} else {
						ProcessScanLine( y, p2, p3, p1, p3, ref col );
					}
				}
			}
		}
		
	}
}
