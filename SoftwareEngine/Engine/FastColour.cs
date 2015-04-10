using System;
using System.Runtime.InteropServices;

namespace SoftwareRasterizer.Engine {
	
	[StructLayout( LayoutKind.Sequential )]
	public struct FastColour {
		
		public int ARGB;
		
        public FastColour( byte r, byte g, byte b, byte a ) {
			ARGB = a << 24 | r << 16 | g << 8 | b;
        }
		
        public FastColour( int r, int g, int b, int a ) {
            ARGB = a << 24 | r << 16 | g << 8 | b;
        }
		
		 public FastColour( byte r, byte g, byte b ) {
			ARGB = 255 << 24 | r << 16 | g << 8 | b;
        }
		
        public FastColour( int r, int g, int b ) {
           ARGB = 255 << 24 | r << 16 | g << 8 | b;
        }
        
        public override bool Equals( object obj ) {
			return ( obj is FastColour ) && Equals( (FastColour)obj );
		}
        
        public bool Equals( FastColour other ) {
        	return ARGB == other.ARGB;
        }
        
        public override int GetHashCode() {
        	return ARGB;
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