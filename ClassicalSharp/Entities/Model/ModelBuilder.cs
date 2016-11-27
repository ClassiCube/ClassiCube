// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;

namespace ClassicalSharp.Model {
	
	public struct BoxDesc {
		public int TexX, TexY, SidesW, BodyW, BodyH;
		public float X1, X2, Y1, Y2, Z1, Z2;
		public float RotX, RotY, RotZ;
		
		/// <summary> Sets the texture origin for this part within the texture atlas. </summary>
		public BoxDesc TexOrigin(int x, int y) {
			TexX = x; TexY = y; return this;
		}
		
		/// <summary> Sets the the two corners of this box, in pixel coordinates. </summary>
		public BoxDesc SetModelBounds(float x1, float y1, float z1, float x2, float y2, float z2) {
			X1 = x1 / 16f; X2 = x2 / 16f;
			Y1 = y1 / 16f; Y2 = y2 / 16f;
			Z1 = z1 / 16f; Z2 = z2 / 16f;
			return this;
		}
		
		/// <summary> Expands the corners of this box outwards by the given amount in pixel units. </summary>
		public BoxDesc Expand(float amount) {
			X1 -= amount / 16f; X2 += amount / 16f;
			Y1 -= amount / 16f; Y2 += amount / 16f;
			Z1 -= amount / 16f; Z2 += amount / 16f;
			return this;
		}
		
		/// <summary> Scales the corners of this box outwards by the given amounts. </summary>
		public BoxDesc Scale(float scale) {
			X1 *= scale; Y1 *= scale; Z1 *= scale;
			X2 *= scale; Y2 *= scale; Z2 *= scale;
			RotX *= scale; RotY *= scale; RotZ *= scale;
			return this;
		}
		
		/// <summary> Sets the point that this box is rotated around, in pixel coordinates. </summary>
		public BoxDesc RotOrigin(sbyte x, sbyte y, sbyte z) {
			RotX = x / 16f; RotY = y / 16f; RotZ = z / 16f;
			return this;
		}
		
		/// <summary> Swaps the min and max X around, resulting in the part being drawn mirrored. </summary>
		public BoxDesc MirrorX() {
			float temp = X1; X1 = X2; X2 = temp;
			return this;
		}
		
		public BoxDesc SetX1(float value) { X1 = value / 16f; return this; }
		
		public BoxDesc SetX2(float value) { X1 = value / 16f; return this; }
	}

	/// <summary> Contains methods to create parts of 3D objects, typically boxes and quads. </summary>
	public static class ModelBuilder {
		
		public static BoxDesc MakeBoxBounds(int x1, int y1, int z1, int x2, int y2, int z2) {
			BoxDesc desc = default(BoxDesc).SetModelBounds(x1, y1, z1, x2, y2, z2);
			desc.SidesW = Math.Abs(z2 - z1);
			desc.BodyW = Math.Abs(x2 - x1);
			desc.BodyH = Math.Abs(y2 - y1);
			return desc;
		}
		
		public static BoxDesc MakeRotatedBoxBounds(int x1, int y1, int z1, int x2, int y2, int z2) {
			BoxDesc desc = default(BoxDesc).SetModelBounds(x1, y1, z1, x2, y2, z2);
			desc.SidesW = Math.Abs(y2 - y1);
			desc.BodyW = Math.Abs(x2 - x1);
			desc.BodyH = Math.Abs(z2 - z1);
			return desc;
		}
		
		/// <summary>Builds a box model assuming the follow texture layout:<br/>
		/// let SW = sides width, BW = body width, BH = body height<br/>
		/// ┏━━━━━━━━━━━━┳━━━━━━━━━━━━━━┳━━━━━━━━━━━━━━┳━━━━━━━━━━━━┓ <br/>
		/// ┃┈┈┈┈┈SW┈┈┈┈┈┃┈┈┈┈┈BW┈┈┈┈┈┃┈┈┈┈┈BW┈┈┈┈┈┈┃┈┈┈┈┈┈┈┈┈┈┈┈┃ <br/>
		/// ┃S┈┈┈┈┈┈┈┈┈┈┈S┃S┈┈┈┈top┈┈┈┈S┃S┈┈bottom┈┈┈S┃┈┈┈┈┈┈┈┈┈┈┈┈┃ <br/>
		/// ┃W┈┈┈┈┈┈┈┈┈W┃W┈┈┈┈tex┈┈┈W┃W┈┈┈┈tex┈┈┈┈W┃┈┈┈┈┈┈┈┈┈┈┈┈┃ <br/>
		/// ┃┈┈┈┈┈SW┈┈┈┈┈┃┈┈┈┈┈BW┈┈┈┈┈┃┈┈┈┈┈BW┈┈┈┈┈┈┃┈┈┈┈┈┈┈┈┈┈┈┈┃ <br/>
		/// ┣━━━━━━━━━━━━━╋━━━━━━━━━━━━━╋━━━━━━━━━━━━━━╋━━━━━━━━━━━━┃ <br/>
		/// ┃┈┈┈┈┈SW┈┈┈┈┈┃┈┈┈┈┈BW┈┈┈┈┈┃┈┈┈┈┈BW┈┈┈┈┈┈┃┈┈┈┈┈SW┈┈┈┈┃ <br/>
		/// ┃B┈┈┈left┈┈┈┈┈B┃B┈┈front┈┈┈┈B┃B┈┈┈right┈┈┈┈B┃B┈┈┈back┈┈┈B┃ <br/>
		/// ┃H┈┈┈tex┈┈┈┈┈H┃H┈┈tex┈┈┈┈┈H┃H┈┈┈tex┈┈┈┈┈H┃H┈┈┈┈tex┈┈┈H┃ <br/>
		/// ┃┈┈┈┈┈SW┈┈┈┈┈┃┈┈┈┈┈BW┈┈┈┈┈┃┈┈┈┈┈BW┈┈┈┈┈┈┃┈┈┈┈┈SW┈┈┈┈┃ <br/>
		/// ┗━━━━━━━━━━━━━┻━━━━━━━━━━━━━┻━━━━━━━━━━━━━┻━━━━━━━━━━━━━┛ </summary>
		public static ModelPart BuildBox(IModel m, BoxDesc desc) {
			int sidesW = desc.SidesW, bodyW = desc.BodyW, bodyH = desc.BodyH;
			float x1 = desc.X1, y1 = desc.Y1, z1 = desc.Z1;
			float x2 = desc.X2, y2 = desc.Y2, z2 = desc.Z2;
			int x = desc.TexX, y = desc.TexY;
			
			YQuad(m, x + sidesW, y, bodyW, sidesW, x2, x1, z2, z1, y2); // top
			YQuad(m, x + sidesW + bodyW, y, bodyW, sidesW, x2, x1, z2, z1, y1); // bottom
			ZQuad(m, x + sidesW, y + sidesW, bodyW, bodyH, x2, x1, y1, y2, z1); // front
			ZQuad(m, x + sidesW + bodyW + sidesW, y + sidesW, bodyW, bodyH, x1, x2, y1, y2, z2); // back
			XQuad(m, x, y + sidesW, sidesW, bodyH, z2, z1, y1, y2, x2); // left
			XQuad(m, x + sidesW + bodyW, y + sidesW, sidesW, bodyH, z1, z2, y1, y2, x1); // right
			return new ModelPart(m.index - 6 * 4, 6 * 4, desc.RotX, desc.RotY, desc.RotZ);
		}
		
		/// <summary>Builds a box model assuming the follow texture layout:<br/>
		/// let SW = sides width, BW = body width, BH = body height<br/>
		/// ┏━━━━━━━━━━━━┳━━━━━━━━━━━━━━┳━━━━━━━━━━━━━━┳━━━━━━━━━━━━┓ <br/>
		/// ┃┈┈┈┈┈SW┈┈┈┈┈┃┈┈┈┈┈BW┈┈┈┈┈┃┈┈┈┈┈BW┈┈┈┈┈┈┃┈┈┈┈┈┈┈┈┈┈┈┈┃ <br/>
		/// ┃S┈┈┈┈┈┈┈┈┈┈┈S┃S┈┈┈┈front┈┈┈S┃S┈┈┈back┈┈┈┈S┃┈┈┈┈┈┈┈┈┈┈┈┈┃ <br/>
		/// ┃W┈┈┈┈┈┈┈┈┈W┃W┈┈┈┈tex┈┈┈W┃W┈┈┈┈tex┈┈┈┈W┃┈┈┈┈┈┈┈┈┈┈┈┈┃ <br/>
		/// ┃┈┈┈┈┈SW┈┈┈┈┈┃┈┈┈┈┈BW┈┈┈┈┈┃┈┈┈┈┈BW┈┈┈┈┈┈┃┈┈┈┈┈┈┈┈┈┈┈┈┃ <br/>
		/// ┣━━━━━━━━━━━━━╋━━━━━━━━━━━━━╋━━━━━━━━━━━━━━╋━━━━━━━━━━━━┃ <br/>
		/// ┃┈┈┈┈┈SW┈┈┈┈┈┃┈┈┈┈┈BW┈┈┈┈┈┃┈┈┈┈┈SW┈┈┈┈┈┈┃┈┈┈┈┈SW┈┈┈┈┃ <br/>
		/// ┃B┈┈┈left┈┈┈┈┈B┃B┈┈bottom┈┈B┃B┈┈┈right┈┈┈┈B┃B┈┈┈┈top┈┈┈B┃ <br/>
		/// ┃H┈┈┈tex┈┈┈┈┈H┃H┈┈tex┈┈┈┈┈H┃H┈┈┈tex┈┈┈┈┈H┃H┈┈┈┈tex┈┈┈H┃ <br/>
		/// ┃┈┈┈┈┈SW┈┈┈┈┈┃┈┈┈┈┈BW┈┈┈┈┈┃┈┈┈┈┈SW┈┈┈┈┈┈┃┈┈┈┈┈SW┈┈┈┈┃ <br/>
		/// ┗━━━━━━━━━━━━━┻━━━━━━━━━━━━━┻━━━━━━━━━━━━━┻━━━━━━━━━━━━━┛ </summary>
		public static ModelPart BuildRotatedBox(IModel m, BoxDesc desc) {
			int sidesW = desc.SidesW, bodyW = desc.BodyW, bodyH = desc.BodyH;
			float x1 = desc.X1, y1 = desc.Y1, z1 = desc.Z1;
			float x2 = desc.X2, y2 = desc.Y2, z2 = desc.Z2;
			int x = desc.TexX, y = desc.TexY;
			
			YQuad(m, x + sidesW + bodyW + sidesW, y + sidesW, bodyW, bodyH, x1, x2, z1, z2, y2); // top
			YQuad(m, x + sidesW, y + sidesW, bodyW, bodyH, x2, x1, z1, z2, y1); // bottom
			ZQuad(m, x + sidesW, y, bodyW, sidesW, x2, x1, y1, y2, z1); // front
			ZQuad(m, x + sidesW + bodyW, y, bodyW, sidesW, x1, x2, y2, y1, z2); // back
			XQuad(m, x, y + sidesW, sidesW, bodyH, y2, y1, z2, z1, x2); // left
			XQuad(m, x + sidesW + bodyW, y + sidesW, sidesW, bodyH, y1, y2, z2, z1, x1); // right
			
			// rotate left and right 90 degrees
			for (int i = m.index - 8; i < m.index; i++) {
				ModelVertex vertex = m.vertices[i];
				float z = vertex.Z; vertex.Z = vertex.Y; vertex.Y = z;
				m.vertices[i] = vertex;
			}
			return new ModelPart(m.index - 6 * 4, 6 * 4, desc.RotX, desc.RotY, desc.RotZ);
		}
		
		public static void XQuad(IModel m, int texX, int texY, int texWidth, int texHeight,
		                     float z1, float z2, float y1, float y2, float x) {
			m.vertices[m.index++] = new ModelVertex(x, y1, z1, texX, texY + texHeight);
			m.vertices[m.index++] = new ModelVertex(x, y2, z1, texX, texY);
			m.vertices[m.index++] = new ModelVertex(x, y2, z2, texX + texWidth, texY);
			m.vertices[m.index++] = new ModelVertex(x, y1, z2, texX + texWidth, texY + texHeight);
		}
		
		public static void YQuad(IModel m, int texX, int texY, int texWidth, int texHeight,
		                     float x1, float x2, float z1, float z2, float y) {
			m.vertices[m.index++] = new ModelVertex(x1, y, z2, texX, texY + texHeight);
			m.vertices[m.index++] = new ModelVertex(x1, y, z1, texX, texY);
			m.vertices[m.index++] = new ModelVertex(x2, y, z1, texX + texWidth, texY);
			m.vertices[m.index++] = new ModelVertex(x2, y, z2, texX + texWidth, texY + texHeight);
		}
		
		public static void ZQuad(IModel m, int texX, int texY, int texWidth, int texHeight,
		                     float x1, float x2, float y1, float y2, float z) {
			m.vertices[m.index++] = new ModelVertex(x1, y1, z, texX, texY + texHeight);
			m.vertices[m.index++] = new ModelVertex(x1, y2, z, texX, texY);
			m.vertices[m.index++] = new ModelVertex(x2, y2, z, texX + texWidth, texY);
			m.vertices[m.index++] = new ModelVertex(x2, y1, z, texX + texWidth, texY + texHeight);
		}
	}
}