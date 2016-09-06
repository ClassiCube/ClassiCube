// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using ClassicalSharp.Entities;
using ClassicalSharp.GraphicsAPI;
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
	
	public class GiantModel : HumanoidModel {
		
		const float size = 2f;
		public GiantModel( Game window ) : base( window ) { }
		
		protected override void MakeDescriptions() {
			base.MakeDescriptions();
			head = head.Scale( size ); torso = torso.Scale( size );
			lLeg = lLeg.Scale( size ); rLeg = rLeg.Scale( size );
			lArm = lArm.Scale( size ); rArm = rArm.Scale( size );
			offset = 0.5f * size;
		}
		
		public override float MaxScale { get { return 1; } }
		
		public override float NameScale { get { return 2; } }

		public override float NameYOffset { get { return 2 * size + 2.2f/16; } }
		
		public override float GetEyeY( Entity entity ) { return base.GetEyeY( entity ) * size; }
		
		public override Vector3 CollisionSize {
			get { return new Vector3( 8/16f * size + 0.6f/16f, 
			                         28.1f/16f * size, 8/16f * size + 0.6f/16f ); }
		}
		
		public override AABB PickingBounds {
			get { return base.PickingBounds.Scale( size ); }
		}
	}
}
