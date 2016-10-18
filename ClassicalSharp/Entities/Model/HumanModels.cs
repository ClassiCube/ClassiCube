// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using ClassicalSharp.Entities;
using ClassicalSharp.GraphicsAPI;
using ClassicalSharp.Physics;
using OpenTK;

namespace ClassicalSharp.Model {

	public class ChibiModel : HumanoidModel {
		
		public ChibiModel( Game window ) : base( window ) { }
		
		const float size = 0.5f;
		protected override void MakeDescriptions() {
			base.MakeDescriptions();
			head = MakeBoxBounds( -4, 12, -4, 4, 20, 4 ).RotOrigin( 0, 13, 0 );
			torso = torso.Scale( size );
			lLeg = lLeg.Scale( size ); rLeg = rLeg.Scale( size );
			lArm = lArm.Scale( size ); rArm = rArm.Scale( size );
			offset = 0.5f * size;
		}

		public override float MaxScale { get { return 3; } }
		
		public override float ShadowScale { get { return 0.5f; } }
		
		public override float NameYOffset { get { return 20.2f/16; } }
		
		public override float GetEyeY( Entity entity ) { return 14/16f; }
		
		public override Vector3 CollisionSize {
			get { return new Vector3( 4/16f + 0.6f/16f, 20.1f/16f, 4/16f + 0.6f/16f ); }
		}
		
		public override AABB PickingBounds {
			get { return new AABB( -4/16f, 0, -4/16f, 4/16f, 16/16f, 4/16f ); }
		}
	}	

	public class HumanoidHeadModel : HumanoidModel {
		
		public HumanoidHeadModel( Game window ) : base( window ) { }
		
		public ModelPart Head, Hat;
		internal override void CreateParts() {
			vertices = new ModelVertex[boxVertices * 2];
			head = MakeBoxBounds( -4, 0, -4, 4, 8, 4 ).RotOrigin( 0, 4, 0 );
			
			Head = BuildBox( head.TexOrigin( 0, 0 ) );
			Hat = BuildBox( head.TexOrigin( 32, 0 ).Expand( offset ) );
		}
		
		public override float NameYOffset { get { return 8/16f + 0.5f/16f; } }
		
		public override float GetEyeY( Entity entity ) { return 6/16f; }
		
		public override Vector3 CollisionSize {
			get { return new Vector3( 7.9f/16f, 7.9f/16f, 7.9f/16f ); }
		}
		
		public override AABB PickingBounds {
			get { return new AABB( -4/16f, 0, -4/16f, 4/16f, 8/16f, 4/16f ); }
		}

		protected override void RenderParts( Player p ) {
			DrawHeadRotate( -p.PitchRadians, 0, 0, Head );
			UpdateVB();
			
			game.Graphics.AlphaTest = true;
			index = 0;
			DrawHeadRotate( -p.PitchRadians, 0, 0, Hat );
			UpdateVB();
		}
	}
}
