// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using ClassicalSharp.Entities;
using ClassicalSharp.GraphicsAPI;
using ClassicalSharp.Physics;
using OpenTK;

namespace ClassicalSharp.Model {

	public class ChibiModel : HumanoidModel {
		
		public ChibiModel(Game window) : base(window) {
			MaxScale = 3.0f;
			ShadowScale = 0.5f;
		}
		
		const float size = 0.5f;
		protected override void MakeDescriptions() {
			base.MakeDescriptions();
			head = MakeBoxBounds(-4, 12, -4, 4, 20, 4).RotOrigin(0, 13, 0);
			torso = torso.Scale(size);
			lLeg = lLeg.Scale(size); rLeg = rLeg.Scale(size);
			lArm = lArm.Scale(size); rArm = rArm.Scale(size);
			offset = 0.5f * size;
		}
		
		public override float NameYOffset { get { return 20.2f/16; } }
		
		public override float GetEyeY(Entity entity) { return 14/16f; }
		
		public override Vector3 CollisionSize {
			get { return new Vector3(4/16f + 0.6f/16f, 20.1f/16f, 4/16f + 0.6f/16f); }
		}
		
		public override AABB PickingBounds {
			get { return new AABB(-4/16f, 0, -4/16f, 4/16f, 16/16f, 4/16f); }
		}
	}	
    
	public class SittingModel : IModel {
		
		public SittingModel(Game window) : base(window) {
			CalcHumanAnims = true;
			ShadowScale = 0.5f;
		}
		
		const int sitOffset = 10;
		public override void CreateParts() { }
		
		public override float NameYOffset { get { return 32/16f + 0.5f/16f; } }
		
		public override float GetEyeY(Entity entity) { return (26 - sitOffset)/16f; }
		
		public override Vector3 CollisionSize {
		    get { return new Vector3(8/16f + 0.6f/16f, (28.1f - sitOffset)/16f, 8/16f + 0.6f/16f); }
		}
		
		public override AABB PickingBounds {
		    get { return new AABB(-8/16f, 0, -4/16f, 8/16f, (32 - sitOffset)/16f, 4/16f); }
		}
		
		protected internal override Matrix4 TransformMatrix(Entity p, Vector3 pos) {
			pos.Y -= (sitOffset / 16f) * p.ModelScale.Y;
			return p.TransformMatrix(p.ModelScale, pos);
		}
		
		public override void DrawModel(Entity p) {
			p.anim.leftLegX = 1.5f; p.anim.rightLegX = 1.5f;
			p.anim.leftLegZ = -0.1f; p.anim.rightLegZ = 0.1f;
			
			IModel model = game.ModelCache.Models[0].Instance;
			model.SetupState(p);
			model.DrawModel(p);
		}		
	}

	public class HumanoidHeadModel : HumanoidModel {
		
		public HumanoidHeadModel(Game window) : base(window) { }
		
		public ModelPart Head, Hat;
		public override void CreateParts() {
			vertices = new ModelVertex[boxVertices * 2];
			head = MakeBoxBounds(-4, 0, -4, 4, 8, 4).RotOrigin(0, 4, 0);
			
			Head = BuildBox(head.TexOrigin(0, 0));
			Hat = BuildBox(head.TexOrigin(32, 0).Expand(offset));
		}
		
		public override float NameYOffset { get { return 8/16f + 0.5f/16f; } }
		
		public override float GetEyeY(Entity entity) { return 6/16f; }
		
		public override Vector3 CollisionSize {
			get { return new Vector3(7.9f/16f, 7.9f/16f, 7.9f/16f); }
		}
		
		public override AABB PickingBounds {
			get { return new AABB(-4/16f, 0, -4/16f, 4/16f, 8/16f, 4/16f); }
		}

		protected override void RenderParts(Entity p) {
			DrawRotate(-p.HeadXRadians, 0, 0, Head, true);
			UpdateVB();
			
			game.Graphics.AlphaTest = true;
			index = 0;
			DrawRotate(-p.HeadXRadians, 0, 0, Hat, true);
			UpdateVB();
		}
	}
}
