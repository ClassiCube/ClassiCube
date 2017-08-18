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
			UsesHumanSkin = true;
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
		public override void CreateParts() { }
		
		public override float GetEyeY(Entity entity) { return 6/16f; }
		
		public override Vector3 CollisionSize {
			get { return new Vector3(7.9f/16f, 7.9f/16f, 7.9f/16f); }
		}
		
		public override AABB PickingBounds {
			get { return new AABB(-4/16f, 0, -4/16f, 4/16f, 8/16f, 4/16f); }
		}
		
		protected internal override Matrix4 TransformMatrix(Entity p, Vector3 pos) {
			pos.Y -= (24 / 16f) * p.ModelScale.Y;
			return p.TransformMatrix(p.ModelScale, pos);
		}

		protected override void RenderParts(Entity p) {
			HumanoidModel human = (HumanoidModel)game.ModelCache.Models[0].Instance;
			vertices = human.vertices;
			
			DrawRotate(-p.HeadXRadians, 0, 0, human.Set.Head, true);
			UpdateVB();
			
			game.Graphics.AlphaTest = true;
			index = 0;
			DrawRotate(-p.HeadXRadians, 0, 0, human.Set.Hat, true);
			UpdateVB();
		}
	}

	public class ArmModel : HumanoidModel {
		
		Matrix4 m;
		public ArmModel(Game window) : base(window) {
			Matrix4.Translate(out m, -6 / 16f, -12 / 16f - 0.1f, 0);
		}
		public override void CreateParts() { }

		public override float NameYOffset { get { return 2.075f; } }
		
		public override float GetEyeY(Entity entity) { return 26/16f; }
		
		public override Vector3 CollisionSize {
			get { return new Vector3(8/16f + 0.6f/16f, 28.1f/16f, 8/16f + 0.6f/16f); }
		}
		
		public override AABB PickingBounds {
			get { return new AABB(-4/16f, 0, -4/16f, 4/16f, 32/16f, 4/16f); }
		}
		
		protected override void RenderParts(Entity p) {
			HumanoidModel human = (HumanoidModel)game.ModelCache.Models[0].Instance;
			vertices = human.vertices;
			
			SkinType skinType = p.SkinType;
			ModelSet model = skinType == SkinType.Type64x64Slim ? human.SetSlim :
				(skinType == SkinType.Type64x64 ? human.Set64 : human.Set);
			
			game.Graphics.PushMatrix();
			game.Graphics.MultiplyMatrix(ref m);
			
			DrawRotate(0, 0, 0 * Utils.Deg2Rad, model.RightArm, false);
			UpdateVB();
			game.Graphics.PopMatrix();
		}
	}
}
