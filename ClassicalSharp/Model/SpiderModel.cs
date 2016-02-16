using System;
using ClassicalSharp.GraphicsAPI;
using OpenTK;

namespace ClassicalSharp.Model {

	public class SpiderModel : IModel {
		
		public SpiderModel( Game window ) : base( window ) {
			vertices = new ModelVertex[boxVertices * 5];
			Head = BuildBox( MakeBoxBounds( -4, 4, -11, 4, 12, -3 )
			                .SetTexOrigin( 32, 4 ) );
			Link = BuildBox( MakeBoxBounds( -3, 5, 3, 3, 11, -3 )
			                .SetTexOrigin( 0, 0 ) );
			End = BuildBox( MakeBoxBounds( -5, 4, 3, 5, 12, 15 )
			               .SetTexOrigin( 0, 12 ) );
			LeftLeg = BuildBox( MakeBoxBounds( -19, 7, -1, -3, 9, 1 )
			                   .SetTexOrigin( 18, 0 ) );
			RightLeg = BuildBox( MakeBoxBounds( 3, 7, -1, 19, 9, 1 )
			                    .SetTexOrigin( 18, 0 ) );
		}
		
		public override bool Bobbing {
			get { return true; }
		}
		
		public override float NameYOffset {
			get { return 1.0125f; }
		}
		
		public override float GetEyeY( Player player ) {
			return 8/16f;
		}
		
		public override Vector3 CollisionSize {
			get { return new Vector3( 15/16f, 12/16f, 15/16f ); }
		}
		
		public override BoundingBox PickingBounds {
			get { return new BoundingBox( -5/16f, 0, -11/16f, 5/16f, 12/16f, 15/16f ); }
		}
		
		const float quarterPi = (float)( Math.PI / 4 );
		const float eighthPi = (float)( Math.PI / 8 );
		protected override void DrawPlayerModel( Player p ) {
			int texId = p.MobTextureId <= 0 ? cache.SpiderTexId : p.MobTextureId;
			graphics.BindTexture( texId );
			DrawHeadRotate( 0, 8/16f, -3/16f, -p.PitchRadians, 0, 0, Head );
			
			DrawPart( Link );
			DrawPart( End );			
			float rotX = (float)(Math.Sin( p.anim.walkTime ) * p.anim.swing * Math.PI);
			float rotZ = (float)(Math.Cos( p.anim.walkTime * 2 ) * p.anim.swing * Math.PI / 16f);
			float rotY = (float)(Math.Sin( p.anim.walkTime * 2 ) * p.anim.swing * Math.PI / 32f);
			Rotate = RotateOrder.XZY;
			
			DrawRotate( -3/16f, 8/16f, 0, rotX, quarterPi + rotY,  eighthPi + rotZ, LeftLeg );
			DrawRotate( -3/16f, 8/16f, 0, -rotX, eighthPi + rotY, eighthPi + rotZ, LeftLeg );
			DrawRotate( -3/16f, 8/16f, 0, rotX, -eighthPi - rotY, eighthPi - rotZ, LeftLeg );
			DrawRotate( -3/16f, 8/16f, 0, -rotX, -quarterPi - rotY, eighthPi - rotZ, LeftLeg );
			DrawRotate( 3/16f, 8/16f, 0, rotX, -quarterPi + rotY, -eighthPi + rotZ, RightLeg );
			DrawRotate( 3/16f, 8/16f, 0, -rotX, -eighthPi + rotY, -eighthPi + rotZ, RightLeg );
			DrawRotate( 3/16f, 8/16f, 0, rotX, eighthPi - rotY, -eighthPi - rotZ, RightLeg );
			DrawRotate( 3/16f, 8/16f, 0, -rotX, quarterPi - rotY, -eighthPi - rotZ, RightLeg );
			Rotate = RotateOrder.ZYX;
			graphics.UpdateDynamicIndexedVb( DrawMode.Triangles, cache.vb, cache.vertices, index, index * 6 / 4 );
		}
		
		ModelPart Head, Link, End, LeftLeg, RightLeg;
	}
}