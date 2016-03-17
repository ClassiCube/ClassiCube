using System;
using System.Drawing;
using ClassicalSharp.GraphicsAPI;
using OpenTK;

namespace ClassicalSharp.Model {

	public class HumanoidModel : IModel {
		
		ModelSet Set, SetSlim;
		public HumanoidModel( Game window ) : base( window ) {
			vertices = new ModelVertex[boxVertices * ( 7 + 2 )];
			Set = new ModelSet();
			
			Set.Head = BuildBox( MakeBoxBounds( -4, 24, -4, 4, 32, 4 )
			                    .SetTexOrigin( 0, 0 ) );
			Set.Torso = BuildBox( MakeBoxBounds( -4, 12, -2, 4, 24, 2 )
			                     .SetTexOrigin( 16, 16 ) );
			Set.LeftLeg = BuildBox( MakeBoxBounds( 0, 0, -2, -4, 12, 2 )
			                       .SetTexOrigin( 0, 16 ) );
			Set.RightLeg = BuildBox( MakeBoxBounds( 0, 0, -2, 4, 12, 2 ).
			                        SetTexOrigin( 0, 16 ) );
			Set.Hat = BuildBox( MakeBoxBounds( -4, 24, -4, 4, 32, 4 )
			                   .SetTexOrigin( 32, 0 ).ExpandBounds( 0.5f ) );
			Set.LeftArm = BuildBox( MakeBoxBounds( -4, 12, -2, -8, 24, 2 )
			                       .SetTexOrigin( 40, 16 ) );
			Set.RightArm = BuildBox( MakeBoxBounds( 4, 12, -2, 8, 24, 2 )
			                        .SetTexOrigin( 40, 16 ) );
			
			SetSlim = new ModelSet();
			SetSlim.Head = Set.Head;
			SetSlim.Torso = Set.Torso;
			SetSlim.LeftLeg = Set.LeftLeg;
			SetSlim.RightLeg = Set.RightLeg;
			SetSlim.LeftArm = BuildBox( MakeBoxBounds( -7, 12, -2, -4, 24, 2 )
			                           .SetTexOrigin( 32, 48 ) );
			SetSlim.RightArm = BuildBox( MakeBoxBounds( 4, 12, -2, 7, 24, 2 )
			                            .SetTexOrigin( 40, 16 ) );
			SetSlim.Hat = Set.Hat;
		}
		
		public override bool Bobbing { get { return true; } }

		public override float NameYOffset { get { return 2.1375f; } }
		
		public override float GetEyeY( Entity entity ) { return 26/16f; }
		
		public override Vector3 CollisionSize {
			get { return new Vector3( 8/16f + 0.6f/16f, 28.1f/16f, 8/16f + 0.6f/16f ); }
		}
		
		public override BoundingBox PickingBounds {
			get { return new BoundingBox( -8/16f, 0, -4/16f, 8/16f, 32/16f, 4/16f ); }
		}
		
		protected override void DrawModel( Player p ) {
			int texId = p.PlayerTextureId <= 0 ? cache.HumanoidTexId : p.PlayerTextureId;
			graphics.BindTexture( texId );
			graphics.AlphaTest = false;
			
			SkinType skinType = p.SkinType;
			_64x64 = skinType != SkinType.Type64x32;
			ModelSet model = skinType == SkinType.Type64x64Slim ? SetSlim : Set;
			DrawHeadRotate( 0, 24/16f, 0, -p.PitchRadians, 0, 0, model.Head );
			
			DrawPart( model.Torso );
			DrawRotate( 0, 12/16f, 0, p.anim.legXRot, 0, 0, model.LeftLeg );
			DrawRotate( 0, 12/16f, 0, -p.anim.legXRot, 0, 0, model.RightLeg );
			Rotate = RotateOrder.XZY;
			DrawRotate( -5/16f, 22/16f, 0, p.anim.leftXRot, 0, p.anim.leftZRot, model.LeftArm );
			DrawRotate( 5/16f, 22/16f, 0, p.anim.rightXRot, 0, p.anim.rightZRot, model.RightArm );
			Rotate = RotateOrder.ZYX;
			graphics.UpdateDynamicIndexedVb( DrawMode.Triangles, cache.vb, cache.vertices, index, index * 6 / 4 );
			
			graphics.AlphaTest = true;
			index = 0;
			DrawHeadRotate( 0, 24f/16f, 0, -p.PitchRadians, 0, 0, model.Hat );
			graphics.UpdateDynamicIndexedVb( DrawMode.Triangles, cache.vb, cache.vertices, index, index * 6 / 4 );
		}
		
		class ModelSet {
			public ModelPart Head, Torso, LeftLeg, RightLeg, LeftArm, RightArm, Hat;
		}
	}
}
