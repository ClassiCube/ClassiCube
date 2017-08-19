// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using ClassicalSharp.Entities;
using ClassicalSharp.Physics;
using OpenTK;

namespace ClassicalSharp.Model {

	public class ChickenModel : IModel {
		
		public ChickenModel(Game window) : base(window) { }
		
		public override void CreateParts() {
			vertices = new ModelVertex[boxVertices * 6 + quadVertices * 2 * 2];
			Head = BuildBox(MakeBoxBounds(-2, 9, -6, 2, 15, -3)
			                .TexOrigin(0, 0)
			                .RotOrigin(0, 9, -4));
			Head2 = BuildBox(MakeBoxBounds(-1, 9, -7, 1, 11, -5)
			                 .TexOrigin(14, 4) // TODO: Find a more appropriate name.
			                 .RotOrigin(0, 9, -4));
			Head3 = BuildBox(MakeBoxBounds(-2, 11, -8, 2, 13, -6)
			                 .TexOrigin(14, 0)
			                 .RotOrigin(0, 9, -4));
			Torso = BuildRotatedBox(MakeRotatedBoxBounds(-3, 5, -4, 3, 11, 3)
			                        .TexOrigin(0, 9));
			
			LeftWing = BuildBox(MakeBoxBounds(-4, 7, -3, -3, 11, 3)
			                    .TexOrigin(24, 13)
			                    .RotOrigin(-3, 11, 0));
			RightWing = BuildBox(MakeBoxBounds(3, 7, -3, 4, 11, 3)
			                     .TexOrigin(24, 13)
			                     .RotOrigin(3, 11, 0));
			
			LeftLeg = MakeLeg(-3, 0, -2, -1);
			RightLeg = MakeLeg(0, 3, 1, 2);
		}
		
		ModelPart MakeLeg(int x1, int x2, int legX1, int legX2) {
			const float y1 = 1/64f, y2 = 5/16f, z2 = 1/16f, z1 = -2/16f;
			ModelBuilder.YQuad(this, 32, 0, 3, 3, x2/16f, x1/16f, z1, z2, y1, false); // bottom feet
			ModelBuilder.ZQuad(this, 36, 3, 1, 5, legX1/16f, legX2/16f, y1, y2, z2, false); // vertical part of leg
			return new ModelPart(index - 2 * 4, 2 * 4, 0/16f, 5/16f, 1/16f);
		}
		
		public override float NameYOffset { get { return 1.0125f; } }
		
		public override float GetEyeY(Entity entity) { return 14/16f; }
		
		public override Vector3 CollisionSize {
			get { return new Vector3(8/16f, 12/16f, 8/16f); }
		}
		
		public override AABB PickingBounds {
			get { return new AABB(-4/16f, 0, -8/16f, 4/16f, 15/16f, 4/16f); }
		}
		
		public override void DrawModel(Entity p) {
			game.Graphics.BindTexture(GetTexture(p));
			DrawRotate(-p.HeadXRadians, 0, 0, Head, true);
			DrawRotate(-p.HeadXRadians, 0, 0, Head2, true);
			DrawRotate(-p.HeadXRadians, 0, 0, Head3, true);

			DrawPart(Torso);
			DrawRotate(0, 0, -Math.Abs(p.anim.leftArmX), LeftWing, false);
			DrawRotate(0, 0, Math.Abs(p.anim.leftArmX), RightWing, false);
			
			int col = cols[0];
			for (int i = 0; i < cols.Length; i++) {
				cols[i] = FastColour.ScalePacked(col, 0.7f);
			}
			DrawRotate(p.anim.leftLegX, 0, 0, LeftLeg, false);
			DrawRotate(p.anim.rightLegX, 0, 0, RightLeg, false);
			UpdateVB();
		}
		
		ModelPart Head, Head2, Head3, Torso, LeftLeg, RightLeg, LeftWing, RightWing;
	}
}