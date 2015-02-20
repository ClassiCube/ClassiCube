using System;
using OpenTK;

namespace ClassicalSharp.Blocks.Model {
	
	public class FluidModel : CubeModel {
		
		public FluidModel( Game game, byte block ) : base( game, block ) {
			NeedsNeighbourState = true;
		}
		
		public override void DrawFace( int face, byte meta, Neighbours state, ref int index, float x, float y, float z, 
		                              VertexPos3fTex2fCol4b[] vertices, FastColour col ) {
			if( ( meta & 0x08 ) != 0 ) {
				base.DrawFace( face, meta, state, ref index, x, y, z, vertices, col );
				return;
			}
			
			//max.Y = 1 - ( meta & 0x7 ) / 16f;
			max.Y = 7 / 8f - meta / 8f;
			/*bool ignoreLeft = state.Left != block;
			bool ignoreRight = state.Right != block;
			bool ignoreFront = state.Front != block;
			bool ignoreBack = state.Back != block;
			
			if( ( ignoreBack || Okay( meta, state.BackMeta ) ) && ( ignoreFront || Okay( meta, state.FrontMeta ) ) &&
			   ( ignoreLeft || Okay( meta, state.LeftMeta ) ) && ( ignoreRight || Okay( meta, state.RightMeta ) ) ) {
				// draw full flat face
				base.DrawFace( face, meta, state, ref index, x, y, z, vertices, col );
			} else {
				Console.WriteLine( "this??" );
			}
			// Appears to be:
			// if W and E meta equal then horizontal
			// if N and S meta equal then vertical
			// else diagonal*/
			base.DrawFace( face, meta, state, ref index, x, y, z, vertices, col );
			max.Y = 1;
		}
		
		/*bool Okay( byte meta, byte neighbourMeta ) {
			return neighbourMeta < 0x08 && ( neighbourMeta == meta + 1 );
		}*/
	}
}
