#region --- License ---
/* Licensed under the MIT/X11 license.
 * Copyright (c) 2006-2008 the OpenTK Team.
 * This notice may not be removed from any source distribution.
 * See license.txt for licensing detailed licensing details.
 */
#endregion

using System;

namespace OpenTK.Graphics
{
    internal class IGraphicsMode
    {
        // Creates a temporary OpenGL context (if necessary) and finds the mode which closest matches
        // the specified parameters.
        public virtual GraphicsMode SelectGraphicsMode(ColorFormat color, int depth, int stencil, int buffers) {
        	  return new GraphicsMode((IntPtr)1, color, depth, stencil, buffers);
        }
    }
}
