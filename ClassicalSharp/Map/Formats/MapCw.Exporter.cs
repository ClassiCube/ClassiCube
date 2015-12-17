using System;
using System.IO;
using System.IO.Compression;
using System.Net;
using System.Text;
using OpenTK;

namespace ClassicalSharp {

	public sealed partial class MapCw : IMapFileFormat {
		
		public override bool SupportsSaving {
			get { return true; }
		}

		BinaryWriter writer;		
		public override void Save( Stream stream, Game game ) {
			using( GZipStream wrapper = new GZipStream( stream, CompressionMode.Compress ) ) {
				writer = new BinaryWriter( wrapper );
				this.game = game;
				map = game.Map;
				
				WriteTag( NbtTagType.Compound ); WriteString( "ClassicWorld" );
				
				WriteTag( NbtTagType.Int8 );
				WriteString( "FormatVersion" ); WriteInt8( 1 );
				
				WriteTag( NbtTagType.Int8Array );
				WriteString( "UUID" ); WriteInt32( 16 );
				WriteBytes( map.Uuid.ToByteArray() );
				
				WriteTag( NbtTagType.Int16 );
				WriteString( "X" ); WriteInt16( (short)map.Width );
				
				WriteTag( NbtTagType.Int16 );
				WriteString( "Y" ); WriteInt16( (short)map.Height );
				
				WriteTag( NbtTagType.Int16 );
				WriteString( "Z" ); WriteInt16( (short)map.Length );
				
				WriteSpawnCompoundTag();
				
				WriteTag( NbtTagType.Int8Array );
				WriteString( "BlockArray" ); WriteInt32( map.mapData.Length );
				WriteBytes( map.mapData );
				
				WriteMetadata();
				
				WriteTag( NbtTagType.End );
			}
		}
		
		void WriteSpawnCompoundTag() {
			WriteTag( NbtTagType.Compound ); WriteString( "Spawn" );
			Vector3 spawn = game.LocalPlayer.Position; // TODO: Maybe also keep spawn too?
			
			WriteTag( NbtTagType.Int16 );
			WriteString( "X" ); WriteInt16( (short)(spawn.X * 32) );
			
			WriteTag( NbtTagType.Int16 );
			WriteString( "Y" ); WriteInt16( (short)(spawn.Y * 32) );
			
			WriteTag( NbtTagType.Int16 );
			WriteString( "Z" ); WriteInt16( (short)(spawn.Z * 32) );
			
			WriteTag( NbtTagType.Int8 );
			WriteString( "H" ); WriteInt8( 0 );
			
			WriteTag( NbtTagType.Int8 );
			WriteString( "P" ); WriteInt8( 0 );
			
			WriteTag( NbtTagType.End );
		}
		
		void WriteMetadata() {
			WriteTag( NbtTagType.Compound ); WriteString( "Metadata" );
			WriteTag( NbtTagType.Compound ); WriteString( "CPE" );
			LocalPlayer p = game.LocalPlayer;

			WriteCpeExtCompound( "ClickDistance", 1 );
			WriteTag( NbtTagType.Int16 );
			WriteString( "Distance" ); WriteInt16( (short)(p.ReachDistance * 32) );
			WriteTag( NbtTagType.End );
			
			WriteCpeExtCompound( "EnvWeatherType", 1 );
			WriteTag( NbtTagType.Int8 );
			WriteString( "WeatherType" ); WriteInt8( (sbyte)((byte)map.Weather) );
			WriteTag( NbtTagType.End );
			
			WriteCpeExtCompound( "EnvColors", 1 );
			WriteColourCompound( "Sky", map.SkyCol );
			WriteColourCompound( "Cloud", map.CloudsCol );
			WriteColourCompound( "Fog", map.FogCol );
			WriteColourCompound( "Ambient", map.Shadowlight );
			WriteColourCompound( "Sunlight", map.Sunlight );
			WriteTag( NbtTagType.End );
			
			WriteTag( NbtTagType.End );
			WriteTag( NbtTagType.End );
		}
		
		void WriteCpeExtCompound( string name, int version ) {
			WriteTag( NbtTagType.Compound ); WriteString( name );
			
			WriteTag( NbtTagType.Int32 );
			WriteString( "ExtensionVersion" ); WriteInt32( 1 );
		}
		
		void WriteColourCompound( string name, FastColour col ) {
			WriteTag( NbtTagType.Compound ); WriteString( name );
			
			WriteTag( NbtTagType.Int16 );
			WriteString( "R" ); WriteInt16( col.R );			
			WriteTag( NbtTagType.Int16 );
			WriteString( "G" ); WriteInt16( col.G );		
			WriteTag( NbtTagType.Int16 );
			WriteString( "B" ); WriteInt16( col.B );
			
			WriteTag( NbtTagType.End );
		}
		
		void WriteTag( NbtTagType v ) { writer.Write( (byte)v ); }
		void WriteInt64( long v ) { writer.Write( IPAddress.HostToNetworkOrder( v ) ); }
		void WriteInt32( int v ) { writer.Write( IPAddress.HostToNetworkOrder( v ) ); }
		void WriteInt16( short v ) { writer.Write( IPAddress.HostToNetworkOrder( v ) ); }
		void WriteInt8( sbyte v ) { writer.Write( (byte)v ); }
		void WriteBytes( byte[] v ) { writer.Write( v ); }
		
		void WriteString( string value ) {
			ushort len = (ushort)value.Length;
			byte[] data = Encoding.UTF8.GetBytes( value );
			WriteInt16( (short)len );
			writer.Write( data );
		}
	}
}