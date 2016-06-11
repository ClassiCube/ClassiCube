#region --- License ---
/* Licensed under the MIT/X11 license.
 * Copyright (c) 2006-2008 the OpenTK Team.
 * This notice may not be removed from any source distribution.
 * See license.txt for licensing detailed licensing details.
 */
#endregion

using System;

namespace OpenTK.Graphics {
	
    /// <summary>Defines the ColorFormat component of a GraphicsMode.</summary>
    /// <remarks> A ColorFormat contains Red, Green, Blue and Alpha components that descibe
    /// the allocated bits per pixel for the corresponding color. </remarks>
    public struct ColorFormat {
        byte red, green, blue, alpha;
        bool isIndexed;
        int bitsPerPixel;

        public ColorFormat(int bpp) {
            if (bpp < 0)
                throw new ArgumentOutOfRangeException("bpp", "Must be greater or equal to zero.");
            red = green = blue = alpha = 0;
            bitsPerPixel = bpp;
            isIndexed = false;

            switch (bpp) {
                case 32:
                    Red = Green = Blue = Alpha = 8;
                    break;
                case 24:
                    Red = Green = Blue = 8;
                    break;
                case 16:
                    Red = Blue = 5;
                    Green = 6;
                    break;
                case 15:
                    Red = Green = Blue = 5;
                    break;
                case 8:
                    Red = Green = 3;
                    Blue = 2;
                    IsIndexed = true;
                    break;
                case 4:
                    Red = Green = 2;
                    Blue = 1;
                    IsIndexed = true;
                    break;
                case 1:
                    IsIndexed = true;
                    break;
                default:
                    Red = Blue = Alpha = (byte)(bpp / 4);
                    Green = (byte)((bpp / 4) + (bpp % 4));
                    break;
            }
        }

        public ColorFormat(int red, int green, int blue, int alpha) {
            if (red < 0 || green < 0 || blue < 0 || alpha < 0)
                throw new ArgumentOutOfRangeException("Arguments must be greater or equal to zero.");
            this.red = (byte)red;
            this.green = (byte)green;
            this.blue = (byte)blue;
            this.alpha = (byte)alpha;
            bitsPerPixel = red + green + blue + alpha;
            isIndexed = bitsPerPixel < 15 && bitsPerPixel != 0;
        }

        /// <summary>Gets the bits per pixel for the Red channel.</summary>
        public int Red { get { return red; } private set { red = (byte)value; } }
        
        /// <summary>Gets the bits per pixel for the Green channel.</summary>
        public int Green { get { return green; } private set { green = (byte)value; } }
        
        /// <summary>Gets the bits per pixel for the Blue channel.</summary>
        public int Blue { get { return blue; } private set { blue = (byte)value; } }
        
        /// <summary>Gets the bits per pixel for the Alpha channel.</summary>
        public int Alpha { get { return alpha; } private set { alpha = (byte)value; } }
        
        /// <summary>Gets a System.Boolean indicating whether this ColorFormat is indexed.</summary>
        public bool IsIndexed { get { return isIndexed; } private set { isIndexed = value; } }
        
        /// <summary>Gets the sum of Red, Green, Blue and Alpha bits per pixel.</summary>
        public int BitsPerPixel { get { return bitsPerPixel; } private set { bitsPerPixel = value; } }

        public static implicit operator ColorFormat(int bpp) {
            return new ColorFormat(bpp);
        }

        public override bool Equals( object obj ) {
			return (obj is ColorFormat) && Equals( (ColorFormat)obj );
		}
        
		public bool Equals(ColorFormat other) {
			return red == other.red && green == other.green && blue == other.blue && alpha == other.alpha;
		}
        
		public static bool operator == (ColorFormat lhs, ColorFormat rhs ) {
			return lhs.Equals( rhs );
		}
        
		public static bool operator != ( ColorFormat lhs, ColorFormat rhs ) {
			return !lhs.Equals( rhs );
		}

        public override int GetHashCode() {
            return Red ^ Green ^ Blue ^ Alpha;
        }
        
        public override string ToString() {
        	return isIndexed ? String.Format( "{0} (indexed)", bitsPerPixel ) : 
        		String.Format( "{0} ({1}{2}{3}{4})", bitsPerPixel, red, green, blue, alpha );
        }
    }
}
