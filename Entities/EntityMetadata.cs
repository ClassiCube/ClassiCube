using System;
using System.Collections.Generic;
using ClassicalSharp.Window;
using ClassicalSharp.Network;
using OpenTK;

namespace ClassicalSharp.Entities {
	
	public class EntityMetadata {
		
		public Dictionary<int, object> Metadata = new Dictionary<int, object>();
		
		public object this[int index] {
			get { return Metadata[index]; }
			set { Metadata[index] = value; }
		}
		
		public static EntityMetadata ReadFrom( NetReader reader ) {
			EntityMetadata dict = new EntityMetadata();
			for( byte item = reader.ReadUInt8(); item != 0x7F; item = reader.ReadUInt8() ) {
				int index = item & 0x1F;
				int type = item >> 5;
				if( type == 0 ) {
					dict[index] = reader.ReadInt8();
				} else if( type == 1 ) {
					dict[index] = reader.ReadInt16();
				} else if( type == 2 ) {
					dict[index] = reader.ReadInt32();
				} else if( type == 3 ) {
					dict[index] = reader.ReadFloat32();
				} else if( type == 4 ) {
					dict[index] = reader.ReadString();
				} else if( type == 5 ) {
					dict[index] = reader.ReadSlot();
				} else if( type == 6 ) {
					int x = reader.ReadInt32();
					int y = reader.ReadInt32();
					int z = reader.ReadInt32();
					dict[index] = new Vector3I( x, y, z );
				}
			}
			return dict;
		}
	}
}
