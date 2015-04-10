using System;
using System.Runtime.InteropServices;

namespace SoftwareRasterizer.Engine {
	
	[StructLayout( LayoutKind.Sequential )]
	public struct FastColour {
		
		public byte A, R, G, B;
		
        public FastColour( byte r, byte g, byte b, byte a ) {
            A = a; 
            R = r; 
            G = g; 
            B = b;
        }
		
        public FastColour( int r, int g, int b, int a ) {
            A = (byte)a; 
            R = (byte)r; 
            G = (byte)g; 
            B = (byte)b;
        }
		
		 public FastColour( byte r, byte g, byte b ) {
			A = 255;
			R = r; 
			G = g; 
			B = b;
        }
		
        public FastColour( int r, int g, int b ) {
            A = 255; 
            R = (byte)r; 
            G = (byte)g; 
            B = (byte)b;
        }
        
        public override string ToString() {
        	return R + ", " + G + ", " + B + " : " + A;
		}
        
        public override bool Equals( object obj ) {
			return ( obj is FastColour ) && Equals( (FastColour)obj );
		}
        
        public bool Equals( FastColour other ) {
        	return A == other.A && R == other.R && G == other.G && B == other.B;
        }
        
        public override int GetHashCode() {
        	return A << 24 | R << 16 | G << 8 | B;
		}

		public static bool operator == ( FastColour left, FastColour right ) {
			return left.Equals( right );
		}
        
		public static bool operator != ( FastColour left, FastColour right ) {
			return !left.Equals( right );
		}
        
        public static FastColour Red = new FastColour( 255, 0, 0 );
        public static FastColour Green = new FastColour( 0, 255, 0 );       
        public static FastColour Blue = new FastColour( 0, 0, 255 );      
        public static FastColour White = new FastColour( 255, 255, 255 );     
        public static FastColour Black = new FastColour( 0, 0, 0 );

        public static FastColour Yellow = new FastColour( 255, 255, 0 );
        public static FastColour Magenta = new FastColour( 255, 0, 255 );
        public static FastColour Cyan = new FastColour( 0, 255, 255 );
    }
}