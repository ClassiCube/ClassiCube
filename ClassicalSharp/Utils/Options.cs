using System;
using System.Collections.Generic;
using System.IO;
using OpenTK;
using OpenTK.Input;

namespace ClassicalSharp {
	
	public static class OptionsKey {
		public const string ViewDist = "viewdist";
		public const string FontSize = "chatfontsize";
		public const string Sensitivity = "mousesensitivity";
		public const string Speed = "speedmultiplier";
		public const string ChatLines = "chatlines";
		
		public const string MouseLeft = "mouseleft";
		public const string MouseMiddle = "mousemiddle";
		public const string MouseRight = "mouseright";
	}
	
	public static class Options {
		
		static Dictionary<string, string> OptionsSet = new Dictionary<string, string>();
		public static bool HasChanged;
		
		public static string Get( string key ) {
			string value;
			return OptionsSet.TryGetValue( key, out value ) ? value : null;
		}
		
		public static int GetInt( string key, int min, int max, int defValue ) {
			string value;
			int valueInt = 0;
			if( !OptionsSet.TryGetValue( key, out value ) || String.IsNullOrEmpty( value )
			   || !Int32.TryParse( value, out valueInt ) )
				return defValue;

			Utils.Clamp( ref valueInt, min, max );
			return valueInt;
		}
		
		public static bool GetBool( string key, bool defValue ) {
			string value;
			bool valueBool = false;
			if( !OptionsSet.TryGetValue( key, out value ) || String.IsNullOrEmpty( value )
			   || !Boolean.TryParse( value, out valueBool ) )
				return defValue;
			return valueBool;
		}
		
		public static Key GetKey( string key, Key defValue ) {
			string value = Options.Get( key );
			if( value == null ) {
				Set( key, defValue );
				return defValue;
			}
			
			Key mapping;
			try {
				mapping = (Key)Enum.Parse( typeof( Key ), value, true );
			} catch( ArgumentException ) {
				Options.Set( key, defValue );
				return defValue;
			}
			return mapping;
		}
		
		public static void Set( string key, string value ) {
			OptionsSet[key] = value;
			HasChanged = true;
		}
		
		public static void Set<T>( string key, T value ) {
			OptionsSet[key] = value.ToString();
			HasChanged = true;
		}
		
		public const string OptionsFile = "options.txt";
		
		public static void Load() {
			try {
				using( Stream fs = File.OpenRead( OptionsFile ) )
					using( StreamReader reader = new StreamReader( fs, false ) )
						LoadFrom( reader );
			} catch( FileNotFoundException ) {
			}
		}
		
		static void LoadFrom( StreamReader reader ) {
			string line;
			while( ( line = reader.ReadLine() ) != null ) {
				if( line.Length == 0 && line[0] == '#' ) continue;
				
				int separatorIndex = line.IndexOf( '=' );
				if( separatorIndex <= 0 ) continue;
				string key = line.Substring( 0, separatorIndex );
				
				separatorIndex++;
				if( separatorIndex == line.Length ) continue;
				string value = line.Substring( separatorIndex, line.Length - separatorIndex );
				OptionsSet[key] = value;
			}
		}
		
		public static void Save() {
			using( Stream fs = File.Create( OptionsFile ) )
				using( StreamWriter writer = new StreamWriter( fs ) )
					SaveTo( writer );
		}
		
		static void SaveTo( StreamWriter writer ) {
			foreach( KeyValuePair<string, string> pair in OptionsSet ) {
				writer.Write( pair.Key );
				writer.Write( '=' );
				writer.Write( pair.Value );
				writer.WriteLine();
			}
		}
	}
}
