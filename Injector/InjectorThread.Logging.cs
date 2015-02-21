using System;
using System.Collections.Generic;
using System.IO;
using System.Reflection;
using System.Text;

namespace Injector {

	public partial class InjectorThread {
		
		StreamWriter c2sWriter, s2cWriter;
		void SetupLogging() {
			DateTime now = DateTime.Now;
			s2cWriter = new StreamWriter( "log " + now.ToString( "dd-MM-yyyy HH-mm-ss" ) + "_s2c.txt" );
			s2cWriter.AutoFlush = true;
			c2sWriter = new StreamWriter( "log " + now.ToString( "dd-MM-yyyy HH-mm-ss" ) + "_c2s.txt" );
			c2sWriter.AutoFlush = true;
			foreach( var constructor in inboundConstructors ) {
				if( constructor == null ) continue;
				PacketS2C packet = constructor();
				Type type = packet.GetType();
				FieldInfo[] fields = type.GetFields( BindingFlags.NonPublic | BindingFlags.Instance );
				s2cPrinters[type] = MakePrinter<PacketS2C>( fields );
			}
			foreach( var constructor in outboundConstructors ) {
				if( constructor == null ) continue;
				PacketC2S packet = constructor();
				Type type = packet.GetType();
				FieldInfo[] fields = type.GetFields( BindingFlags.NonPublic | BindingFlags.Instance );
				c2sPrinters[type] = MakePrinter<PacketC2S>( fields );
			}
		}
		
		Action<T, StringBuilder> MakePrinter<T>( FieldInfo[] fields ) {
			return (T p, StringBuilder printer) => {
				FieldInfo[] fields2 = fields;
				for( int i = 0; i < fields2.Length; i++ ) {
					FieldInfo field = fields2[i];
					object value = field.GetValue( p );
					if( value is byte[] ) {
						byte[] arrayValue = ( value as byte[] );
						printer.AppendLine( field.Name + ": ("+ arrayValue.Length + " bytes) " +
						                   Utils.ToSpacedHexString( arrayValue ) );
					} else if( value is Slot[] ) {
						Slot[] slotsValue = ( value as Slot[] );
						printer.AppendLine( field.Name + ": ("+ slotsValue.Length + " items)" );
						for( int j = 0; j < slotsValue.Length; j++ ) {
							printer.AppendLine( "   " + j + ": " + slotsValue[j].ToString() );
						}
					} else {
						printer.AppendLine( field.Name + ": " + value.ToString() );
					}
					
				}
			};
		}
		
		const string format = "[{0} (0x{1},{2})]";
		void LogC2S( byte opcode, PacketC2S packet ) {
			Type type = packet.GetType();
			if( IgnoreC2S( type ) ) return;
			c2sWriter.WriteLine(
				String.Format( format,
				              DateTime.Now.ToString( "HH:mm:ss" ),
				              opcode.ToString( "X2" ),
				              type.Name )
			);
			StringBuilder builder = new StringBuilder();
			c2sPrinters[type]( packet, builder );
			builder.AppendLine();
			c2sWriter.WriteLine( builder.ToString() );
		}
		
		void LogS2C( byte opcode, PacketS2C packet ) {
			Type type = packet.GetType();
			if( IgnoreS2C( type ) ) return;
			s2cWriter.WriteLine(
				String.Format( format,
				              DateTime.Now.ToString( "HH:mm:ss" ),
				              opcode.ToString( "X2" ),
				              type.Name )
			);
			StringBuilder builder = new StringBuilder();
			s2cPrinters[type]( packet, builder );
			s2cWriter.WriteLine( builder.ToString() );
		}
		
		bool IgnoreS2C( Type type ) {
			return type == typeof( MapChunkInbound ) || type == typeof( PrepareChunkInbound ) ||
				type == typeof( MapsInbound );
		}
		
		bool IgnoreC2S( Type type ) {
			return type == typeof( PlayerPosAndLookOutbound ) || type == typeof( PlayerOutbound ) ||
				type == typeof( PlayerPosOutbound ) || type == typeof( PlayerLookOutbound );
		}
		
		Dictionary<Type, Action<PacketC2S, StringBuilder>> c2sPrinters = new Dictionary<Type, Action<PacketC2S, StringBuilder>>();
		Dictionary<Type, Action<PacketS2C, StringBuilder>> s2cPrinters = new Dictionary<Type, Action<PacketS2C, StringBuilder>>();
	}
}
