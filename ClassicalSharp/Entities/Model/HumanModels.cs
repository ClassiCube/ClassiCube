// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using ClassicalSharp.Entities;
using ClassicalSharp.GraphicsAPI;
using ClassicalSharp.Physics;
using OpenTK;

namespace ClassicalSharp.Model {

	public class ChibiModel : HumanoidModel {
		
		public ChibiModel(Game window) : base(window) { }
		
		const float size = 0.5f;
		protected override void MakeDescriptions() {
			base.MakeDescriptions();
			head = MakeBoxBounds(-4, 12, -4, 4, 20, 4).RotOrigin(0, 13, 0);
			torso = torso.Scale(size);
			lLeg = lLeg.Scale(size); rLeg = rLeg.Scale(size);
			lArm = lArm.Scale(size); rArm = rArm.Scale(size);
			offset = 0.5f * size;
		}

		public override float MaxScale { get { return 3; } }
		
		public override float ShadowScale { get { return 0.5f; } }
		
		public override float NameYOffset { get { return 20.2f/16; } }
		
		public override float GetEyeY(Entity entity) { return 14/16f; }
		
		public override Vector3 CollisionSize {
			get { return new Vector3(4/16f + 0.6f/16f, 20.1f/16f, 4/16f + 0.6f/16f); }
		}
		
		public override AABB PickingBounds {
			get { return new AABB(-4/16f, 0, -4/16f, 4/16f, 16/16f, 4/16f); }
		}
	}	
    
	public class SittingModel : HumanoidModel {
		
		public SittingModel(Game window) : base(window) { }
		
		const int sitOffset = 10;
		protected override void MakeDescriptions() {
		    head = MakeBoxBounds(-4, 24-sitOffset, -4, 4, 32-sitOffset, 4).RotOrigin(0, (sbyte)(24-sitOffset), 0);
			torso = MakeBoxBounds(-4, 12-sitOffset, -2, 4, 24-sitOffset, 2);
			lLeg = MakeBoxBounds(-4, 0-sitOffset, -2, 0, 12-sitOffset, 2).RotOrigin(0, (sbyte)(12-sitOffset), 0);
			rLeg = MakeBoxBounds(0, 0-sitOffset, -2, 4, 12-sitOffset, 2).RotOrigin(0, (sbyte)(12-sitOffset), 0);
			lArm = MakeBoxBounds(-8, 12-sitOffset, -2, -4, 24-sitOffset, 2).RotOrigin(-5, (sbyte)(22-sitOffset), 0);
			rArm = MakeBoxBounds(4, 12-sitOffset, -2, 8, 24-sitOffset, 2).RotOrigin(5, (sbyte)(22-sitOffset), 0);
		}

		public override float MaxScale { get { return 2; } }
		
		public override float ShadowScale { get { return 0.5f; } }
		
		public override float NameYOffset { get { return (32-sitOffset)/16f + 0.5f/16f; } }
		
		public override float GetEyeY(Entity entity) { return (26-sitOffset)/16f; }
		
		public override Vector3 CollisionSize {
		    get { return new Vector3(8/16f + 0.6f/16f, (28.1f-sitOffset)/16f, 8/16f + 0.6f/16f); }
		}
		
		public override AABB PickingBounds {
		    get { return new AABB(-8/16f, 0, -4/16f, 8/16f, (32-sitOffset)/16f, 4/16f); }
		}
		
		protected override void DrawModel(Entity p) {
			game.Graphics.BindTexture(GetTexture(p.TextureId));
			game.Graphics.AlphaTest = false;
			
			bool _64x64 = p.SkinType != SkinType.Type64x32;
			uScale = p.uScale / 64f;
			vScale = p.vScale / (_64x64 ? 64 : 32);
			RenderParts(p);
		}
		
		protected override void RenderParts(Entity p) {
			SkinType skinType = p.SkinType;
			ModelSet model = skinType == SkinType.Type64x64Slim ? SetSlim :
				(skinType == SkinType.Type64x64 ? Set64 : Set);
			
			DrawRotate(-p.HeadXRadians, 0, 0, model.Head, true);
			DrawPart(model.Torso);
			
			float legRotateX = 1.5f;
			float legRotateZ = 0.1f;
			
			DrawRotate(legRotateX, 0, -legRotateZ, model.LeftLeg, false);
			DrawRotate(legRotateX, 0, legRotateZ, model.RightLeg, false);
			Rotate = RotateOrder.XZY;
			//DrawRotate(0, 0, -legRotateZ, model.LeftArm, false);
			//DrawRotate(0, 0, legRotateZ, model.RightArm, false);
			DrawRotate(p.anim.leftXRot, 0, p.anim.leftZRot, model.LeftArm, false);
			DrawRotate(p.anim.rightXRot, 0, p.anim.rightZRot, model.RightArm, false);
			Rotate = RotateOrder.ZYX;
			UpdateVB();
			
			game.Graphics.AlphaTest = true;
			index = 0;
			if (skinType != SkinType.Type64x32) {
				DrawPart(model.TorsoLayer);
				DrawRotate(legRotateX, 0, -legRotateZ, model.LeftLegLayer, false);
				DrawRotate(legRotateX, 0, legRotateZ, model.RightLegLayer, false);
				Rotate = RotateOrder.XZY;
				
				DrawRotate(p.anim.leftXRot, 0, p.anim.leftZRot, model.LeftArmLayer, false);
				DrawRotate(p.anim.rightXRot, 0, p.anim.rightZRot, model.RightArmLayer, false);
				//DrawRotate(0, 0, -legRotateZ, model.LeftArmLayer, false);
				//DrawRotate(0, 0, legRotateZ, model.RightArmLayer, false);
				Rotate = RotateOrder.ZYX;
			}
			DrawRotate(-p.HeadXRadians, 0, 0, model.Hat, true);
			UpdateVB();
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
