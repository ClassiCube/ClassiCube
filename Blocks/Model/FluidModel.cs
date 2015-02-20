using System;

namespace ClassicalSharp.Blocks.Model {
	
	public class FluidModel : VariableHeightCubeModel {
		
		public FluidModel( Game game, byte block ) : base( game, block ) {
			NeedsAdjacentNeighbours = true;
			NeedsDiagonalNeighbours = true;		
			NeedsAllAboveNeighbours = true;
		}
		
		public override bool FaceHidden( int face, byte meta, ref Neighbours state, byte neighbour ) {
			return ( face == TileSide.Top && neighbour != block ) ? false : info.IsFaceHidden( block, neighbour, face );
		}
		
		public override void DrawFace( int face, byte meta, ref Neighbours state, ref int index, float x, float y, float z,
		                              VertexPos3fTex2fCol4b[] vertices, FastColour col ) {
			bool useLeft = state.Left == block;
			bool useRight = state.Right == block;
			bool useFront = state.Front == block;
			bool useBack = state.Back == block;
			bool useLeftFront = state.LeftFront == block;
			bool useRightFront = state.RightFront == block;
			bool useLeftBack = state.LeftBack == block;
			bool useRightBack = state.RightBack == block;
			
			heightX0Z0 = Math.Max( useLeft ? Height( state.LeftMeta, state.AboveLeft ) : 0,
			                      Math.Max( useFront ? Height( state.FrontMeta, state.AboveFront ) : 0,
			                               Math.Max( useLeftFront ? Height( state.LeftFrontMeta, state.AboveLeftFront ) : 0,
			                                        Height( meta, state.Above ) ) ) );
			
			heightX1Z0 = Math.Max( useRight ? Height( state.RightMeta, state.AboveRight ) : 0,
			                      Math.Max( useFront ? Height( state.FrontMeta, state.AboveFront ) : 0,
			                               Math.Max( useRightFront ? Height( state.RightFrontMeta, state.AboveRightFront ) : 0,
			                                        Height( meta, state.Above ) ) ) );
			
			heightX0Z1 = Math.Max( useLeft ? Height( state.LeftMeta, state.AboveLeft ) : 0,
			                      Math.Max( useBack ? Height( state.BackMeta, state.AboveBack ) : 0,
			                               Math.Max( useLeftBack ? Height( state.LeftBackMeta, state.AboveLeftBack ) : 0,
			                                        Height( meta, state.Above ) ) ) );
			
			heightX1Z1 = Math.Max( useRight ? Height( state.RightMeta, state.AboveRight ) : 0,
			                      Math.Max( useBack ? Height( state.BackMeta, state.AboveBack ) : 0,
			                               Math.Max( useRightBack ? Height( state.RightBackMeta, state.AboveRightBack ) : 0,
			                                        Height( meta, state.Above ) ) ) );
			
			base.DrawFace( face, meta, ref state, ref index, x, y, z, vertices, col );
		}
		
		float Height( byte meta, byte above ) {
			return above == block ? 1 : 7 / 8f - ( meta & 0x07 ) / 8f;
		}
	}
}
