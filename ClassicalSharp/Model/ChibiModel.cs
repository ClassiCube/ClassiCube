using System;
using ClassicalSharp.GraphicsAPI;
using OpenTK;

namespace ClassicalSharp.Model {

	public class ChibiModel : IModel {
		
		ModelSet Set, SetSlim;
		public ChibiModel( Game window ) : base( window ) {
			vertices = new ModelVertex[boxVertices * (7 + 2)];
			Set = new ModelSet();
			
			Set.Head = EasyBox(8, 8, 8, 8f, 8f, 8f, 0, 0, 0, 16, 0);
			Set.Hat  = EasyBox(8, 8, 8, 8.5f, 8.5f, 8.5f, 32, 0, 0, 16, 0);
			
			Set.Torso = EasyBox(8, 12, 4, 4f, 6f, 2f, 16, 16, 0, 9, 0);
			
			Set.LeftLeg = EasyBox(4, 12, 4, -2f, 6f, 2f, 0, 16, -1, 3, 0);
			
			Set.RightLeg = EasyBox(4, 12, 4, 2f, 6f, 2f, 0, 16, 1, 3, 0);
			
			
			Set.LeftArm = EasyBox(4, 12, 4, -2f, 6f, 2f, 40, 16, -3, 9, 0);

			Set.RightArm = EasyBox(4, 12, 4, 2f, 6f, 2f, 40, 16, 3, 9, 0);
		}
		
		public override bool Bobbing { get { return true; } }

		public override float NameYOffset { get { return 2.1375f; } }
		
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
			DrawHeadRotate( 0, 13/16f, 0, -p.PitchRadians, 0, 0, Set.Head );
			
			DrawPart( Set.Torso );
			DrawRotate( 0, 6/16f, 0, p.anim.legXRot, 0, 0, Set.LeftLeg );
			DrawRotate( 0, 6/16f, 0, -p.anim.legXRot, 0, 0, Set.RightLeg );
			Rotate = RotateOrder.XZY;
			
			DrawRotate( -3/16f, 11/16f, 0, p.anim.leftXRot, 0, p.anim.leftZRot, Set.LeftArm );
			DrawRotate( 3/16f, 11/16f, 0, p.anim.rightXRot, 0, p.anim.rightZRot, Set.RightArm );
			Rotate = RotateOrder.ZYX;
			
			graphics.UpdateDynamicIndexedVb( DrawMode.Triangles, cache.vb, cache.vertices, index, index * 6 / 4 );
			
			graphics.AlphaTest = true;
			index = 0;
			DrawHeadRotate( 0, 13f/16f, 0, -p.PitchRadians, 0, 0, Set.Hat );
			graphics.UpdateDynamicIndexedVb( DrawMode.Triangles, cache.vb, cache.vertices, index, index * 6 / 4 );
		}
		
		protected ModelPart EasyBox( int xSize, int ySize, int zSize, float xResize, float yResize, float zResize,
		                                 int u, int v, int xOff = 0, int yOff = 0, int zOff = 0 ) {
			return BuildBox(
				MakeBoxBounds( (int)(-xSize * 0.5f + xOff), (int)(-ySize * 0.5f + yOff), (int)(-zSize * 0.5f + zOff),
				              (int)(xSize * 0.5f + xOff),  (int)(ySize * 0.5f + yOff),  (int)(zSize * 0.5f + zOff) )
				.SetTexOrigin( u, v )
				.SetModelBounds( -xResize * 0.5f + xOff, -yResize * 0.5f + yOff, -zResize * 0.5f + zOff,
				                xResize * 0.5f + xOff,  yResize * 0.5f + yOff,  zResize * 0.5f + zOff )
			);
		}
		
		class ModelSet {
			public ModelPart Head, Torso, LeftLeg, RightLeg, LeftArm, RightArm, Hat;
		}
	}
}
