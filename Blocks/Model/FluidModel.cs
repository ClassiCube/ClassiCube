using System;
using OpenTK;

namespace ClassicalSharp.Blocks.Model {
	
	public class FluidModel : VariableHeightCubeModel {
		
		public FluidModel( Game game, byte block ) : base( game, block ) {
			NeedsAdjacentNeighbours = true;
			NeedsDiagonalNeighbours = true;
			heightX0Z0 = heightX0Z1 = heightX1Z0 = heightX1Z1 = 14 / 16f;
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
			
			heightX0Z0 = Math.Max( Height( useLeft ? state.LeftMeta : meta ),
			                      Math.Max( Height( useFront ? state.FrontMeta : meta ),
			                               Math.Max( Height( useLeftFront ? state.LeftFrontMeta : meta ),
			                                        Height( meta ) ) ) );
			
			heightX1Z0 = Math.Max( Height( useRight ? state.RightMeta : meta ),
			                      Math.Max( Height( useFront ? state.FrontMeta : meta ),
			                               Math.Max( Height( useRightFront ? state.RightFrontMeta : meta ),
			                                        Height( meta ) ) ) );
			
			heightX0Z1 = Math.Max( Height( useLeft ? state.LeftMeta : meta ),
			                      Math.Max( Height( useBack ? state.BackMeta : meta ),
			                               Math.Max( Height( useLeftBack ? state.LeftBackMeta : meta ),
			                                        Height( meta ) ) ) );
			
			heightX1Z1 = Math.Max( Height( useRight ? state.RightMeta : meta ),
			                      Math.Max( Height( useBack ? state.BackMeta : meta ),
			                               Math.Max( Height( useRightBack ? state.RightBackMeta : meta ),
			                                        Height( meta ) ) ) );
			
			base.DrawFace( face, meta, ref state, ref index, x, y, z, vertices, col );
			heightX0Z0 = heightX0Z1 = heightX1Z0 = heightX1Z1 = 14 / 16f;
		}
		
		float Height( byte meta ) {
			return 7 / 8f - ( meta & 0x07 ) / 8f;
		}
		
		float Height( byte meta, byte above ) {
			return above == block ? 1 : 7 / 8f - ( meta & 0x07 ) / 8f;
		}
	}
}
