// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using ClassicalSharp.Entities;
using ClassicalSharp.GraphicsAPI;
using ClassicalSharp.Physics;
using OpenTK;

namespace ClassicalSharp.Model {

	public class CreeperModel : IModel {
		
		public CreeperModel( Game window ) : base( window ) { }
		
		internal override void CreateParts() {
			vertices = new ModelVertex[boxVertices * 6];
			Head =  BuildBox( MakeBoxBounds( -4, 18, -4, 4, 26, 4 )
			                 .TexOrigin( 0, 0 )
			                 .RotOrigin( 0, 18, 0 ) );
			Torso = BuildBox( MakeBoxBounds( -4, 6, -2, 4, 18, 2 )
			                 .TexOrigin( 16, 16 ) );
			
			LeftLegFront = BuildBox( MakeBoxBounds( -4, 0, -6, 0, 6, -2 )
			                        .TexOrigin( 0, 16 )
			                        .RotOrigin( 0, 6, -2 ) );
			RightLegFront = BuildBox( MakeBoxBounds( 0, 0, -6, 4, 6, -2 )
			                         .TexOrigin( 0, 16 )
			                         .RotOrigin( 0, 6, -2 ) );
			LeftLegBack = BuildBox( MakeBoxBounds( -4, 0, 2, 0, 6, 6 )
			                       .TexOrigin( 0, 16 )
			                       .RotOrigin( 0, 6, 2 ) );
			RightLegBack = BuildBox( MakeBoxBounds( 0, 0, 2, 4, 6, 6 )
			                        .TexOrigin( 0, 16 )
			                        .RotOrigin( 0, 6, 2 ) );
		}
		
		public override bool Bobbing { get { return true; } }
		
		public override float NameYOffset { get { return 1.7f; } }
		
		public override float GetEyeY( Entity entity ) { return 22/16f; }
		
		public override Vector3 CollisionSize {
			get { return new Vector3( 8/16f, 26/16f, 8/16f ); }
		}
		
		public override AABB PickingBounds {
			get { return new AABB( -4/16f, 0, -6/16f, 4/16f, 26/16f, 6/16f ); }
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