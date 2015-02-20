using System;
using OpenTK;

namespace ClassicalSharp.Blocks.Model {
	
	public class FluidModel : VariableHeightCubeModel {
		
		public FluidModel( Game game, byte block ) : base( game, block ) {
			NeedsNeighbourState = true;
			heightX0Z0 = heightX0Z1 = heightX1Z0 = heightX1Z1 = 14 / 16f;
		}
		
		public override void DrawFace( int face, byte meta, Neighbours state, ref int index, float x, float y, float z,
		                              VertexPos3fTex2fCol4b[] vertices, FastColour col ) {
			if( meta >= 0x08 ) {
				if( state.Above == block ) {
					heightX0Z0 = heightX0Z1 = heightX1Z0 = heightX1Z1 = 1;
				}
				base.DrawFace( face, meta, state, ref index, x, y, z, vertices, col );
				heightX0Z0 = heightX0Z1 = heightX1Z0 = heightX1Z1 = 14 / 16f;
				return;
			}
			
			bool useLeft = state.Left == block;
			bool useRight = state.Right == block;
			bool useFront = state.Front == block;
			bool useBack = state.Back == block;
			// Do side X0 Z0
			if( useLeft && useFront ) {
				heightX0Z0 = ( Math.Max( Height( meta, state.LeftMeta ), Height( meta, state.FrontMeta ) ) + Height( meta ) ) / 2f;
			} else if( useLeft ) {
				heightX0Z0 = ( Height( meta, state.LeftMeta ) + Height( meta ) ) / 2f;
			} else if( useFront ) {
				heightX0Z0 = ( Height( meta, state.FrontMeta ) + Height( meta ) ) / 2f;
			} else {
				heightX0Z0 = Height( meta );
			}
			// Do side X1 Z0
			if( useRight && useFront ) {
				heightX1Z0 = ( Math.Max( Height( meta, state.RightMeta ), Height( meta, state.FrontMeta ) ) + Height( meta ) ) / 2f;
			} else if( useRight ) {
				heightX1Z0 = ( Height( meta, state.RightMeta ) + Height( meta ) ) / 2f;
			} else if( useFront ) {
				heightX1Z0 = ( Height( meta, state.FrontMeta ) + Height( meta ) ) / 2f;
			} else {
				heightX1Z0 = Height( meta );
			}
			// Do side X0 Z1
			if( useLeft && useBack ) {
				heightX0Z1 = ( Math.Max( Height( meta, state.LeftMeta ), Height( meta, state.BackMeta ) ) + Height( meta ) ) / 2f;
			} else if( useLeft ) {
				heightX0Z1 = ( Height( meta, state.LeftMeta ) + Height( meta ) ) / 2f;
			} else if( useBack ) {
				heightX0Z1 = ( Height( meta, state.BackMeta ) + Height( meta ) ) / 2f;
			} else {
				heightX0Z1 = Height( meta );
			}
			// Do side X1 Z1
			if( useRight && useBack ) {
				heightX1Z1 = ( Math.Max( Height( meta, state.RightMeta ), Height( meta, state.BackMeta ) ) + Height( meta ) ) / 2f;
			} else if( useRight ) {
				heightX1Z1 = ( Height( meta, state.RightMeta ) + Height( meta ) ) / 2f;
			} else if( useBack ) {
				heightX1Z1 = ( Height( meta, state.BackMeta ) + Height( meta ) ) / 2f;
			} else {
				heightX1Z1 = Height( meta );
			}
			
			base.DrawFace( face, meta, state, ref index, x, y, z, vertices, col );
			heightX0Z0 = heightX0Z1 = heightX1Z0 = heightX1Z1 = 14 / 16f;
		}
		
		static float Height( byte meta, byte neighbourMeta ) {
			if( neighbourMeta >= 0x08 ) return 7 / 8f - meta / 8f;
			return 7 / 8f - neighbourMeta / 8f;
		}
		
		static float Height( byte meta ) {
			return 7 / 8f - meta / 8f;
		}
	}
}
