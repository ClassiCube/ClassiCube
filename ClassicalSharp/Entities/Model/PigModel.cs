// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using ClassicalSharp.Entities;
using ClassicalSharp.GraphicsAPI;
using OpenTK;

namespace ClassicalSharp.Model {

	public class PigModel : IModel {
		
		public PigModel( Game window ) : base( window ) { }
		
		internal override void CreateParts() {
			vertices = new ModelVertex[boxVertices * 6];
			Head = BuildBox( MakeBoxBounds( -4, 8, -14, 4, 16, -6 )
			               .TexOrigin( 0, 0 )
			               .RotOrigin( 0, 12, -6 ) );
			Torso = BuildRotatedBox( MakeRotatedBoxBounds( -5, 6, -8, 5, 14, 8 )
			                        .TexOrigin( 28, 8 ) );
			LeftLegFront = BuildBox( MakeBoxBounds( -5, 0, -7, -1, 6, -3 )
			                        .TexOrigin( 0, 16 )
			                        .RotOrigin( 0, 6, -5 ) );
			RightLegFront = BuildBox( MakeBoxBounds( 1, 0, -7, 5, 6, -3 )
			                         .TexOrigin( 0, 16 )
			                         .RotOrigin( 0, 6, -5 ) );
			LeftLegBack = BuildBox( MakeBoxBounds( -5, 0, 5, -1, 6, 9 )
			                       .TexOrigin( 0, 16 )
			                       .RotOrigin( 0, 6, 7 ) );
			RightLegBack = BuildBox( MakeBoxBounds( 1, 0, 5, 5, 6, 9 )
			                        .TexOrigin( 0, 16 )
			                        .RotOrigin( 0, 6, 7 ) );
		}
		
		public override bool Bobbing { get { return true; } }
		
		public override float NameYOffset { get { return 1.075f; } }
		
		public override float GetEyeY( Entity entity ) { return 12/16f; }
		
		public override Vector3 CollisionSize {
			get { return new Vector3( 14/16f, 14/16f, 14/16f ); }
		}
		
		public override AABB PickingBounds {
			get { return new AABB( -5/16f, 0, -14/16f, 5/16f, 16/16f, 9/16f ); }
		}
		
		protected override void DrawModel( Player p ) {
			game.Graphics.BindTexture( GetTexture( p.MobTextureId ) );
			DrawHeadRotate( -p.PitchRadians, 0, 0, Head );

			DrawPart( Torso );
			DrawRotate( p.anim.legXRot, 0, 0, LeftLegFront );
			DrawRotate( -p.anim.legXRot, 0, 0, RightLegFront );
			DrawRotate( -p.anim.legXRot, 0, 0, LeftLegBack );
			DrawRotate( p.anim.legXRot, 0, 0, RightLegBack );
			UpdateVB();
		}
		
		ModelPart Head, Torso, LeftLegFront, RightLegFront, LeftLegBack, RightLegBack;
	}
}