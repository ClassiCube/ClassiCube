// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using ClassicalSharp.Entities;
using ClassicalSharp.GraphicsAPI;
using ClassicalSharp.Network;
using OpenTK;

#if FALSE
namespace ClassicalSharp.Model {

	public class CustomModel : IModel {
		
		public CustomModel(Game window) : base(window) { }
		
		internal override void CreateParts() { }
		
		internal bool bobbing;
		public override bool Bobbing { get { return bobbing; } }

		internal float nameYOffset;
		public override float NameYOffset { get { return nameYOffset; } }
		
		internal float eyeY;
		public override float GetEyeY(Entity entity) { return eyeY; }
		
		internal Vector3 collisonSize;
		public override Vector3 CollisionSize { get { return collisonSize; } }
		
		internal AABB pickingBounds;
		public override AABB PickingBounds { get { return pickingBounds; } }
		
		protected override void DrawModel(Player p) {
			int texId = p.TextureId <= 0 ? cache.HumanoidTexId : p.TextureId;
		}
		
		internal void ReadSetupPacket() {
			
		}
		
		internal void ReadMetadataPacket() {
			collisonSize = ReadS16Vec3(reader);
			pickingBounds.Min = ReadS16Vec3(reader);
			pickingBounds.Max = ReadS16Vec3(reader);
			nameYOffset = reader.ReadInt16() / 256f;
			eyeY = reader.ReadInt16() / 256f;
			bobbing = reader.ReadUInt8() != 0;
		}
		
		internal void ReadDefinePartPacket() {
			ushort partId = reader.ReadUInt16();
			byte type = reader.ReadUInt8();
			Vector3 min = ReadS16Vec3(reader);
			Vector3 max = ReadS16Vec3(reader);
		}
		
		internal void ReadRotationPacket() {
			ushort partId = reader.ReadUInt16();
			byte order = reader.ReadUInt8();
			RotateData rotX = ReadRotateData(reader);
			RotateData rotY = ReadRotateData(reader);
			RotateData rotZ = ReadRotateData(reader);
		}
		
		CustomModelPart[] parts;
		Vector3 ReadS16Vec3(NetReader reader) {
			return new Vector3(reader.ReadInt16() / 256f, reader.ReadInt16() / 256f,
			                   reader.ReadInt16() / 256f);
		}
		
		RotateData ReadRotateData(NetReader reader) {
			RotateData data = default(RotateData);
			data.Origin = reader.ReadInt16() / 256f;
			data.Type = reader.ReadUInt8();
			data.VarA = reader.ReadInt16() / 512f;
			data.VarB = reader.ReadInt16() / 512f;
			return data;
		}
		
		struct CustomModelPart {
			public RotateOrder Order;
			public RotateData RotX, RotY, RotZ;
		}
		
		struct RotateData {
			public float Origin;
			public byte Type;
			public float VarA, VarB;
		}
	}
}
#endif
