using System;
using System.Collections.Generic;
using System.Text;

namespace Injector {
	
	public class EntityMetadata {
		
		public Dictionary<int, object> Metadata = new Dictionary<int, object>();
		
		public static EntityMetadata ReadFrom( StreamInjector reader ) {
			EntityMetadata dict = new EntityMetadata();
			for( byte item = reader.ReadUInt8(); item != 0x7F; item = reader.ReadUInt8() ) {
				int index = item & 0x1F;
				int type = item >> 5;
				if( type == 0 ) {
					dict.Metadata[index] = reader.ReadInt8();
				} else if( type == 1 ) {
					dict.Metadata[index] = reader.ReadInt16();
				} else if( type == 2 ) {
					dict.Metadata[index] = reader.ReadInt32();
				} else if( type == 3 ) {
					dict.Metadata[index] = reader.ReadFloat32();
				} else if( type == 4 ) {
					dict.Metadata[index] = reader.ReadString();
				} else if( type == 5 ) {
					dict.Metadata[index] = reader.ReadSlot();
				} else if( type == 6 ) {
					int x = reader.ReadInt32();
					int y = reader.ReadInt32();
					int z = reader.ReadInt32();
					dict.Metadata[index] = new Vector3I( x, y, z );
				}
			}
			return dict;
		}
		
		public override string ToString() {
			StringBuilder builder = new StringBuilder();
			foreach( var pair in Metadata ) {
				builder.AppendLine( "   " + pair.Key + ": " + pair.Value );
			}
			return builder.ToString();
		}
	}
	
	public struct Vector3I {
		public int X, Y, Z;
		
		public Vector3I( int x, int y, int z ) {
			X = x;
			Y = y;
			Z = z;
		}
		
		public override string ToString() {
			return X + "," + Y + "," + Z;
		}
	}
}
