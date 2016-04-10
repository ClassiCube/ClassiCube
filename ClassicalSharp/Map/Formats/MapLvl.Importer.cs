// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
// Part of fCraft | Copyright (c) 2009-2014 Matvei Stefarov <me@matvei.org> | BSD-3 | See LICENSE.txt
using System;
using System.IO;
using System.IO.Compression;
using System.Text;
using ClassicalSharp.Entities;

namespace ClassicalSharp.Map {

	/// <summary> Imports a world from a LVL map file (MCLawl server map) </summary>
	public sealed class MapLvl : IMapFormatImporter {

		const int Version = 1874;
		public byte[] Load( Stream stream, Game game, out int width, out int height, out int length ) {
			using( GZipStream gs = new GZipStream( stream, CompressionMode.Decompress ) ) {
				BinaryReader reader = new BinaryReader( gs );
                ushort header = reader.ReadUInt16();

                width = header == Version ? reader.ReadUInt16() : header;
                length = reader.ReadUInt16();
                height = reader.ReadUInt16();
                
                LocalPlayer p = game.LocalPlayer;
                p.Spawn.X = reader.ReadUInt16();
                p.Spawn.Z = reader.ReadUInt16();
                p.Spawn.Y = reader.ReadUInt16();
                p.SpawnYaw = (float)Utils.PackedToDegrees( reader.ReadByte() );
                p.SpawnPitch = (float)Utils.PackedToDegrees( reader.ReadByte() );
                
                if( header == Version )
                	reader.ReadUInt16(); // pervisit and perbuild perms
                byte[] blocks = new byte[width * height * length];
				int read = gs.Read( blocks, 0, blocks.Length );
				
				// TODO: Fallback for physics blocks
				// TODO: Custom blocks
				return blocks;
			}
		}
	}
}