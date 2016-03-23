using System;
using ClassicalSharp.GraphicsAPI;
using OpenTK;

namespace ClassicalSharp.Model {

	public class ChibiModel : IModel {
		
		public ChibiModel( Game window ) : base( window ) {
			vertices = new ModelVertex[boxVertices * (7 + 2)];
			
			Head = BuildBox( MakeBoxBounds( -4, 12, -4, 4, 20, 4 )
			                .SetTexOrigin( 0, 0 )
			                .SetRotOrigin( 0, 13, 0 ) );
			Hat = BuildBox( MakeBoxBounds( -4, 12, -4, 4, 20, 4 )
			               .SetTexOrigin( 32, 0 ).ExpandBounds( 0.25f )
			               .SetRotOrigin( 0, 13, 0 ) );
			Torso = BuildBox( MakeBoxBounds( -4, -6, -2, 4, 6, 2 )
			                 .SetTexOrigin( 16, 16 )
			                 .SetModelBounds( -2, 6, -1, 2, 12, 1 ) );
			LeftLeg = BuildBox( MakeBoxBounds( -2, -6, -2, 2, 6, 2 )
			                   .SetTexOrigin( 0, 16 )
			                   .SetModelBounds( -0, 0, -1, -2, 6, 1 )
			                   .SetRotOrigin( 0, 6, 0 ) );
			RightLeg = BuildBox( MakeBoxBounds( -2, -6, -2, 2, 6, 2 )
			                    .SetTexOrigin( 0, 16 )
			                    .SetModelBounds( 0, 0, -1, 2, 6, 1 )
			                    .SetRotOrigin( 0, 6, 0 ) );
			LeftArm = BuildBox( MakeBoxBounds( -2, -6, -2, 2, 6, 2 )
			                   .SetTexOrigin( 40, 16 )
			                   .SetModelBounds( -2, 6, -1, -4, 12, 1 )
			                   .SetRotOrigin( -3, 11, 0 ) );
			RightArm = BuildBox( MakeBoxBounds( -2, -6, -2, 2, 6, 2 )
			                    .SetTexOrigin( 40, 16 )
			                    .SetModelBounds( 2, 6, -1, 4, 12, 1 )
			                    .SetRotOrigin( 3, 11, 0 ) );
		}
		
		public override bool Bobbing { get { return true; } }

		public override float NameYOffset { get { return 1.3875f; } }
		
		public override float GetEyeY( Entity entity ) { return 14/16f; }
		
		public override Vector3 CollisionSize {
			get { return new Vector3( 8/16f + 0.6f/16f, 20.1f/16f, 8/16f + 0.6f/16f ); }
		}
		
		public override BoundingBox PickingBounds {
			get { return new BoundingBox( -4/16f, 0, -4/16f, 4/16f, 16/16f, 4/16f ); }
		}
		
		protected override void DrawModel( Player p ) {
			int texId = p.PlayerTextureId <= 0 ? cache.HumanoidTexId : p.PlayerTextureId;
			graphics.BindTexture( texId );
			graphics.AlphaTest = false;
			
			SkinType skinType = p.SkinType;
			_64x64 = skinType != SkinType.Type64x32;
			DrawHeadRotate( -p.PitchRadians, 0, 0, Head );
			
			DrawPart( Torso );
			DrawRotate( p.anim.legXRot, 0, 0, LeftLeg );
			DrawRotate( -p.anim.legXRot, 0, 0, RightLeg );
			Rotate = RotateOrder.XZY;			
			DrawRotate( p.anim.leftXRot, 0, p.anim.leftZRot, LeftArm );
			DrawRotate( p.anim.rightXRot, 0, p.anim.rightZRot, RightArm );
			Rotate = RotateOrder.ZYX;			
			graphics.UpdateDynamicIndexedVb( DrawMode.Triangles, cache.vb, cache.vertices, index, index * 6 / 4 );
			
			graphics.AlphaTest = true;
			index = 0;
			DrawHeadRotate( -p.PitchRadians, 0, 0, Hat );
			graphics.UpdateDynamicIndexedVb( DrawMode.Triangles, cache.vb, cache.vertices, index, index * 6 / 4 );
		}
		
		ModelPart Head, Torso, LeftLeg, RightLeg, LeftArm, RightArm, Hat;
	}
}
