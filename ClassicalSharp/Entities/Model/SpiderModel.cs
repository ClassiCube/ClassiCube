// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using ClassicalSharp.Entities;
using ClassicalSharp.GraphicsAPI;
using OpenTK;

namespace ClassicalSharp.Model {

	public class SpiderModel : IModel {
		
		public SpiderModel( Game window ) : base( window ) { }
		
		internal override void CreateParts() {
			vertices = new ModelVertex[boxVertices * 5];
			Head = BuildBox( MakeBoxBounds( -4, 4, -11, 4, 12, -3 )
			                .TexOrigin( 32, 4 )
			                .RotOrigin( 0, 8, -3 ) );
			Link = BuildBox( MakeBoxBounds( -3, 5, 3, 3, 11, -3 )
			                .TexOrigin( 0, 0 ) );
			End = BuildBox( MakeBoxBounds( -5, 4, 3, 5, 12, 15 )
			               .TexOrigin( 0, 12 ) );
			LeftLeg = BuildBox( MakeBoxBounds( -19, 7, -1, -3, 9, 1 )
			                   .TexOrigin( 18, 0 )
			                   .RotOrigin( -3, 8, 0 ) );
			RightLeg = BuildBox( MakeBoxBounds( 3, 7, -1, 19, 9, 1 )
			                    .TexOrigin( 18, 0 )
			                    .RotOrigin( 3, 8, 0 ) );
		}
		
		public override bool Bobbing { get { return true; } }
		
		public override float NameYOffset { get { return 1.0125f; } }
		
		public override float GetEyeY( Entity entity ) { return 8/16f; }
		
		public override Vector3 CollisionSize {
			get { return new Vector3( 15/16f, 12/16f, 15/16f ); }
		}
		
		public override AABB PickingBounds {
			get { return new AABB( -5/16f, 0, -11/16f, 5/16f, 12/16f, 15/16f ); }
		}
		
		const float quarterPi = (float)( Math.PI / 4 );
		const float eighthPi = (float)( Math.PI / 8 );
		protected override void DrawModel( Player p ) {
			game.Graphics.BindTexture( GetTexture( p.MobTextureId ) );
			DrawHeadRotate( -p.PitchRadians, 0, 0, Head );
			
			DrawPart( Link );
			DrawPart( End );			
			float rotX = (float)(Math.Sin( p.anim.walkTime ) * p.anim.swing * Math.PI);
			float rotZ = (float)(Math.Cos( p.anim.walkTime * 2 ) * p.anim.swing * Math.PI / 16f);
			float rotY = (float)(Math.Sin( p.anim.walkTime * 2 ) * p.anim.swing * Math.PI / 32f);
			Rotate = RotateOrder.XZY;
			
			DrawRotate( rotX, quarterPi + rotY,  eighthPi + rotZ, LeftLeg );
			DrawRotate( -rotX, eighthPi + rotY, eighthPi + rotZ, LeftLeg );
			DrawRotate( rotX, -eighthPi - rotY, eighthPi - rotZ, LeftLeg );
			DrawRotate( -rotX, -quarterPi - rotY, eighthPi - rotZ, LeftLeg );
			DrawRotate( rotX, -quarterPi + rotY, -eighthPi + rotZ, RightLeg );
			DrawRotate( -rotX, -eighthPi + rotY, -eighthPi + rotZ, RightLeg );
			DrawRotate( rotX, eighthPi - rotY, -eighthPi - rotZ, RightLeg );
			DrawRotate( -rotX, quarterPi - rotY, -eighthPi - rotZ, RightLeg );
			Rotate = RotateOrder.ZYX;
			UpdateVB();
		}		
		ModelPart Head, Link, End, LeftLeg, RightLeg;
	}
}