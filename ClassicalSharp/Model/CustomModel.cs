using System;
using ClassicalSharp.GraphicsAPI;
using OpenTK;

namespace ClassicalSharp.Model {

	public class CustomModel : IModel {
		
		public CustomModel( Game window ) : base( window ) {
		}
		
		internal bool bobbing;
		public override bool Bobbing { get { return bobbing; } }

		internal float nameYOffset;
		public override float NameYOffset { get { return nameYOffset; } }
		
		internal float eyeY;
		public override float GetEyeY( Entity entity ) { return eyeY; }
		
		internal Vector3 collisonSize;
		public override Vector3 CollisionSize { get { return collisonSize; } }
		
		internal BoundingBox pickingBounds;
		public override BoundingBox PickingBounds { get { return pickingBounds; } }
		
		protected override void DrawModel( Player p ) {
			int texId = p.PlayerTextureId <= 0 ? cache.HumanoidTexId : p.PlayerTextureId;
		}
		
		internal void ReadMetadataPacket( NetReader reader ) {
			collisonSize = ReadVector( reader );
			pickingBounds.Min = ReadVector( reader );
			pickingBounds.Max = ReadVector( reader );
			nameYOffset = reader.ReadInt16() / 256f;
			eyeY = reader.ReadInt16() / 256f;
			bobbing = reader.ReadUInt8() != 0;
		}
		
		internal void ReadDefinePartPacket( NetReader reader ) {
			ushort partId = reader.ReadUInt16();
			byte type = reader.ReadUInt8();
			Vector3 min = ReadVector( reader );
			Vector3 max = ReadVector( reader );			
		}
		
		internal void ReadRotationPacket( NetReader reader ) {
			ushort partId = reader.ReadUInt16();
			byte order = reader.ReadUInt8();
			RotateData rotX = ReadRotateData( reader );
			RotateData rotY = ReadRotateData( reader );
			RotateData rotZ = ReadRotateData( reader );
		}
		
		CustomModelPart[] parts;
		Vector3 ReadVector( NetReader reader ) {
			return new Vector3( reader.ReadInt16() / 256f, reader.ReadInt16() / 256f,
			                   reader.ReadInt16() / 256f );
		}
		
		RotateData ReadRotateData( NetReader reader ) {
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
