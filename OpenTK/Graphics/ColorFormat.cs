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

        public ColorFormat(int bpp) {
            if (bpp < 0)
                throw new ArgumentOutOfRangeException("bpp", "Must be greater or equal to zero.");
            Red = Green = Blue = Alpha = 0;
            BitsPerPixel = bpp;
            IsIndexed = false;

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
            Red = (byte)red;
            Green = (byte)green;
            Blue = (byte)blue;
            Alpha = (byte)alpha;
            BitsPerPixel = red + green + blue + alpha;
            IsIndexed = BitsPerPixel < 15 && BitsPerPixel != 0;
        }

        /// <summary> Gets the bits per pixel for the Red channel. </summary>
        public byte Red;
        
        /// <summary> Gets the bits per pixel for the Green channel. </summary>
        public byte Green;
        
        /// <summary>Gets the bits per pixel for the Blue channel.</summary>
        public byte Blue;
        
        /// <summary>Gets the bits per pixel for the Alpha channel.</summary>
        public byte Alpha;
        
        /// <summary> Gets a System.Boolean indicating whether this ColorFormat is indexed. </summary>
        public bool IsIndexed;
        
        /// <summary> Gets the sum of Red, Green, Blue and Alpha bits per pixel.</summary>
        public int BitsPerPixel;

        public static implicit operator ColorFormat(int bpp) {
            return new ColorFormat(bpp);
        }
    }
}
