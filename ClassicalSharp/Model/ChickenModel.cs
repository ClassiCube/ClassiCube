using System;
using ClassicalSharp.GraphicsAPI;
using OpenTK;

namespace ClassicalSharp.Model {

	public class ChickenModel : IModel {
		
		public ChickenModel( Game window ) : base( window ) {
			vertices = new ModelVertex[boxVertices * 6 + quadVertices * 2 * 2];
			Head = BuildBox( MakeBoxBounds( -2, 9, -6, 2, 15, -3 )
			                .SetTexOrigin( 0, 0 ) );
			Head2 = BuildBox( MakeBoxBounds( -1, 9, -7, 1, 11, -5 )
			                 .SetTexOrigin( 14, 4 ) ); // TODO: Find a more appropriate name.
			Head3 = BuildBox( MakeBoxBounds( -2, 11, -8, 2, 13, -6 )
			                 .SetTexOrigin( 14, 0 ) );
			Torso = BuildRotatedBox( MakeRotatedBoxBounds( -3, 5, -4, 3, 11, 3 )
			                       .SetTexOrigin( 0, 9 ) );
			
			LeftLeg = MakeLeg( -3, 0, -2, -1 );
			RightLeg = MakeLeg( 0, 3, 1, 2 );
			LeftWing = BuildBox( MakeBoxBounds( -4, 7, -3, -3, 11, 3 )
			                    .SetTexOrigin( 24, 13 ) );
			RightWing = BuildBox( MakeBoxBounds( 3, 7, -3, 4, 11, 3 )
			                     .SetTexOrigin( 24, 13 ) );
		}
		
		ModelPart MakeLeg( int x1, int x2, int legX1, int legX2 ) {
			const float y1 = 1/64f, y2 = 5/16f, z2 = 1/16f, z1 = -2/16f;
			YQuad( 32, 0, 3, 3, x2/16f, x1/16f, z1, z2, y1 ); // bottom feet
			ZQuad( 36, 3, 1, 5, legX1/16f, legX2/16f, y1, y2, z2 ); // vertical part of leg
			return new ModelPart( index - 2 * 4, 2 * 4 );
		}
		
		public override bool Bobbing {
			get { return true; }
		}
		
		public override float NameYOffset {
			get { return 1.0125f; }
		}
		
		public override float GetEyeY( Player player ) {
			return 14/16f;
		}
		
		public override Vector3 CollisionSize {
			get { return new Vector3( 8/16f, 12/16f, 8/16f ); }
		}
		
		public override BoundingBox PickingBounds {
			get { return new BoundingBox( -4/16f, 0, -8/16f, 4/16f, 15/16f, 4/16f ); }
		}
		
		protected override void DrawPlayerModel( Player p ) {
			int texId = p.MobTextureId <= 0 ? cache.ChickenTexId : p.MobTextureId;
			graphics.BindTexture( texId );
			
			cosA = (float)Math.Cos( p.HeadYawRadians );
			sinA = (float)Math.Sin( p.HeadYawRadians );
			DrawRotate( 0, 9/16f, -3/16f, -p.PitchRadians, 0, 0, Head );
			DrawRotate( 0, 9/16f, -3/16f, -p.PitchRadians, 0, 0, Head2 );
			DrawRotate( 0, 9/16f, -3/16f, -p.PitchRadians, 0, 0, Head3 );
			
			cosA = (float)Math.Cos( p.YawRadians );
			sinA = (float)Math.Sin( p.YawRadians );
			DrawPart( Torso );
			DrawRotate( 0, 5/16f, 1/16f, p.legXRot, 0, 0, LeftLeg );
			DrawRotate( 0, 5/16f, 1/16f, -p.legXRot, 0, 0, RightLeg );
			DrawRotate( -3/16f, 11/16f, 0, 0, 0, -Math.Abs( p.armXRot ), LeftWing );
			DrawRotate( 3/16f, 11/16f, 0, 0, 0, Math.Abs( p.armXRot ), RightWing );
			graphics.UpdateDynamicIndexedVb( DrawMode.Triangles, cache.vb, cache.vertices, index, index * 6 / 4 );
		}
		
		ModelPart Head, Head2, Head3, Torso, LeftLeg, RightLeg, LeftWing, RightWing;
	}
}