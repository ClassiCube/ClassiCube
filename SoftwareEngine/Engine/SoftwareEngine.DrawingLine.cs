using System;
using OpenTK;

namespace SoftwareRasterizer.Engine {
	
	public unsafe partial class SoftwareEngine {
		
		public void Line_DrawVertexLine( Vector3 vertex1, Vector3 vertex2 ) {
			float x1 = 0, y1 = 0, x2 = 0, y2 = 0;
			Project( ref vertex1, out x1, out y1 );
			Project( ref vertex2, out x2, out y2 );
			DrawLine( x1, y1, x2, y2 );
		}
		
		public void Line_DrawIndexedTriangles( Vector3[] vertices, ushort[] indices ) {
			for( int i = 0; i < indices.Length; i += 3 ) {
				Vector3 vertex1 = vertices[indices[i + 0]];
				Vector3 vertex2 = vertices[indices[i + 1]];
				Vector3 vertex3 = vertices[indices[i + 2]];

				float x1 = 0, y1 = 0, x2 = 0, y2 = 0, x3 = 0, y3 = 0;
				Project( ref vertex1, out x1, out y1 );
				Project( ref vertex2, out x2, out y2 );
				Project( ref vertex3, out x3, out y3 );
				DrawLine( x1, y1, x2, y2 );
				DrawLine( x2, y2, x3, y3 );
				DrawLine( x3, y3, x1, y1 );
			}
		}
		
		const int INSIDE = 0; // 0000
		const int LEFT = 1;   // 0001
		const int RIGHT = 2;  // 0010
		const int BOTTOM = 4; // 0100
		const int TOP = 8;    // 1000		
		// Compute the bit code for a point (x, y) using the clip rectangle
		// bounded diagonally by (xmin, ymin), and (xmax, ymax)
		int ComputeOutCode( float x, float y ) {	
			int code = INSIDE;
			
			if( x < 0 ) {
				code |= LEFT;
			} else if( x > xMax ) {
				code |= RIGHT;
			}
			if( y < 0 ) {
				code |= BOTTOM;
			} else if ( y > yMax ) {
				code |= TOP;
			}
			return code;
		}
		
		void DrawLine( float x0, float y0, float x1, float y1 ) {
			int outcode0 = ComputeOutCode( x0, y0 );
			int outcode1 = ComputeOutCode( x1, y1 );
			
			while( true ) {
				if( ( outcode0 | outcode1 ) == 0 ) {
					myLine( (int)x0, (int)y0, (int)x1, (int)y1 );
					return;
				} else if( ( outcode0 & outcode1 ) != 0 ) {
					return;
				} else {
					// failed both tests, so calculate the line segment to clip
					// from an outside point to an intersection with clip edge
					float x = 0, y = 0;
					int outcodeOut = outcode0 != 0 ? outcode0 : outcode1;
					
					// use formulas y = y0 + slope * (x - x0), x = x0 + (1 / slope) * (y - y0)
					if( ( outcodeOut & TOP ) != 0 ) {
						x = x0 + ( x1 - x0 ) * ( yMax - y0 ) / ( y1 - y0 );
						y = yMax;
					} else if( ( outcodeOut & BOTTOM ) != 0 ) {
						x = x0 + ( x1 - x0 ) * ( 0 - y0 ) / ( y1 - y0 );
						y = 0;
					} else if( ( outcodeOut & RIGHT ) != 0 ) {
						y = y0 + ( y1 - y0 ) * ( xMax - x0 ) / ( x1 - x0 );
						x = xMax;
					} else if( ( outcodeOut & LEFT ) != 0 ) {
						y = y0 + ( y1 - y0 ) * ( 0 - x0 ) / ( x1 - x0 );
						x = 0;
					}
					
					if( outcodeOut == outcode0 ) {
						x0 = x;
						y0 = y;
						outcode0 = ComputeOutCode( x0, y0 );
					} else {
						x1 = x;
						y1 = y;
						outcode1 = ComputeOutCode( x1, y1 );
					}
				}
			}
		}

		// Freely useable in non-commercial applications as long as
		// credits to Po-Han Lin and link to http://www.edepot.com is
		// provided in source code and can been seen in compiled executable.
		// Commercial applications please inquire about licensing the algorithms.

		// THE EXTREMELY FAST LINE ALGORITHM Variation D (Addition Fixed Point)
		void myLine( int x1, int y1, int x2, int y2 ) {
			bool yLonger = false;
			int incrementVal;
			int shortLen = y2 - y1;
			int longLen= x2 - x1;
			
			if( Math.Abs( shortLen ) > Math.Abs( longLen ) ) {
				int swap = shortLen;
				shortLen= longLen;
				longLen = swap;
				yLonger = true;
			}
			
			int endVal = longLen;
			if( longLen < 0 ) {
				incrementVal = -1;
				longLen = -longLen;
			} else {
				incrementVal = 1;
			}
			
			int decInc = longLen == 0 ? 0 : ( shortLen << 16 ) / longLen;
			int j = 0;
			if( yLonger ) {
				for( int i = 0; i != endVal; i += incrementVal ) {
					int xx = x1 + ( j >> 16 );
					int yy = y1 + i;
					if( xx >= 0 && yy >= 0 && xx < width && yy < height )  {
						PutPixel( xx, yy, FastColour.Red );
					}
					j += decInc;
				}
			} else {
				for( int i = 0; i != endVal; i += incrementVal ) {
					int xx = x1 + i;
					int yy = y1 + ( j >> 16 );
					if( xx >= 0 && yy >= 0 && xx < width && yy < height )  {
						PutPixel( xx, yy, FastColour.Red );
					}
					j += decInc;
				}
			}
		}
		
		void DrawLin32e( int x0, int y0, int x1, int y1 ) {			
			int dx = Math.Abs( x1 - x0 );
			int dy = Math.Abs( y1 - y0 );
			int dirX = ( x0 < x1 ) ? 1 : -1;
			int dirY = ( y0 < y1 ) ? 1 : -1;
			int err = dx - dy;

			while( true ) {
				if( x0 >= 0 && y0 >= 0 && x0 < width && y0 < height )  {
					PutPixel( x0, y0, FastColour.Red );
				}
				if ( ( x0 == x1 ) && ( y0 == y1 ) ) break;
				
				int e2 = 2 * err;
				if ( e2 > -dy ) {
					err -= dy;
					x0 += dirX;
				}
				if( e2 < dx ) {
					err += dx;
					y0 += dirY;
				}
			}
		}
	}
}
