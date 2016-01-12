using System;
using System.Collections.Generic;
using System.IO;
using ClassicalSharp.TexturePack;
using OpenTK;
using OpenTK.Input;

namespace ClassicalSharp {
	
	public static class OptionsKey {
		
		public const string ViewDist = "viewdist";
		public const string HudScale = "hudscale";
		public const string ChatScale = "chatscale";
		public const string Sensitivity = "mousesensitivity";
		public const string Speed = "speedmultiplier";
		public const string ChatLines = "chatlines";
		public const string ClickableChat = "chatclickable";
		public const string ArialChatFont = "arialchatfont";
		public const string DefaultTexturePack = "defaulttexpack";
		public const string SingleplayerPhysics = "singleplayerphysics";
		public const string ShowBlockInHand = "blockinhand";
		public const string UseMusic = "usemusic";
		public const string UseSound = "usesound";
		public const string HacksEnabled = "hacksenabled";
		public const string NamesMode = "namesmode";
		
		public const string MouseLeft = "mouseleft";
		public const string MouseMiddle = "mousemiddle";
		public const string MouseRight = "mouseright";
		public const string FpsLimit = "fpslimit";
		public const string AutoCloseLauncher = "autocloselauncher";
		public const string ViewBobbing = "viewbobbing";
		public const string FieldOfView = "fov";
		public const string LiquidsBreakable = "liquidsbreakable";
		public const string PushbackPlacing = "pushbackplacing";
		public const string InvertMouse = "invertmouse";
		public const string NoclipSlide = "noclipslide";
		public const string CameraClipping = "cameraclipping";
		public const string DoubleJump = "doublejump";
		public const string TabAutocomplete = "tab-autocomplete";
		
		public const string AllowCustomBlocks = "nostalgia-customblocks";
		public const string UseCPE = "nostalgia-usecpe";
		public const string AllowServerTextures = "nostalgia-servertextures";
		public const string UseClassicGui = "nostalgia-classicgui";
		public const string SimpleArmsAnim = "nostalgia-simplearms";
		public const string UseClassicTabList = "nostalgia-classictablist";
	}
	
	// TODO: implement this
	public enum FpsLimitMethod {
		LimitVSync,
		Limit30FPS,
		Limit60FPS,
		Limit120FPS,		
		LimitNone,
	}
	
	public static class Options {
		
		public static Dictionary<string, string> OptionsSet = new Dictionary<string, string>();
		public static bool HasChanged;
		const string OptionsFile = "options.txt";
		
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
		
		public static float GetFloat( string key, float min, float max, float defValue ) {
			string value;
			float valueFloat = 0;
			if( !OptionsSet.TryGetValue( key, out value ) || String.IsNullOrEmpty( value )
			   || !Single.TryParse( value, out valueFloat ) )
				return defValue;

			Utils.Clamp( ref valueFloat, min, max );
			return valueFloat;
		}
		
		public static T GetEnum<T>( string key, T defValue ) {
			string value = Options.Get( key.ToLower() );
			if( value == null ) {
				Set( key, defValue );
				return defValue;
			}
			
			T mapping;
			if( !Utils.TryParseEnum( value, defValue, out mapping ) )
				Options.Set( key, defValue );
			return mapping;
		}
		
		public static void Set( string key, string value ) {
			key = key.ToLower();
			if( value != null ) {
				OptionsSet[key] = value;
			} else {
				OptionsSet.Remove( key );
			}
			HasChanged = true;
		}
		
		public static void Set<T>( string key, T value ) {
			key = key.ToLower();
			OptionsSet[key] = value.ToString();
			HasChanged = true;
		}
		
		public static bool Load() {
			// Both of these are from when running from the launcher
			if( Program.AppDirectory == null )
				Program.AppDirectory = AppDomain.CurrentDomain.BaseDirectory;
			string defZip = Path.Combine( Program.AppDirectory, "default.zip" );
			string texDir = Path.Combine( Program.AppDirectory, TexturePackExtractor.Dir );
			if( File.Exists( defZip ) || !Directory.Exists( texDir ) )
				Program.CleanupMainDirectory();			   
			
			try {
				string path = Path.Combine( Program.AppDirectory, OptionsFile );
				using( Stream fs = File.OpenRead( path ) )
					using( StreamReader reader = new StreamReader( fs, false ) )
						LoadFrom( reader );
				return true;
			} catch( FileNotFoundException ) {
				return true;
			} catch( IOException ex ) {
				ErrorHandler.LogError( "loading options", ex );
				return false;
			}
		}
		
		static void LoadFrom( StreamReader reader ) {
			string line;
			while( (line = reader.ReadLine()) != null ) {
				if( line.Length == 0 && line[0] == '#' ) continue;
				
				int separatorIndex = line.IndexOf( '=' );
				if( separatorIndex <= 0 ) continue;
				string key = line.Substring( 0, separatorIndex );
				key = key.ToLower();
				
				separatorIndex++;
				if( separatorIndex == line.Length ) continue;
				string value = line.Substring( separatorIndex, line.Length - separatorIndex );
				OptionsSet[key] = value;
			}
		}
		
		public static bool Save() {
			try {
				string path = Path.Combine( Program.AppDirectory, OptionsFile );
				using( Stream fs = File.Create( path ) )
					using( StreamWriter writer = new StreamWriter( fs ) )
						SaveTo( writer );
				return true;
			} catch( IOException ex ) {
				ErrorHandler.LogError( "saving options", ex );
				return false;
			}
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
