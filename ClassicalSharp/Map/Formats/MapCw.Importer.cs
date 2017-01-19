// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using System.IO;
using System.IO.Compression;
using ClassicalSharp.Entities;
using ClassicalSharp.Network;
using ClassicalSharp.Network.Protocols;
using OpenTK;
using NbtCompound = System.Collections.Generic.Dictionary<string, ClassicalSharp.Map.NbtTag>;

namespace ClassicalSharp.Map {

	public sealed class MapCwImporter : IMapFormatImporter {
		
		BinaryReader reader;
		NbtFile file;
		Game game;
		World map;
		
		public byte[] Load(Stream stream, Game game, out int width, out int height, out int length) {
			GZipHeaderReader gsHeader = new GZipHeaderReader();
			while (!gsHeader.ReadHeader(stream)) { }
			
			using (DeflateStream gs = new DeflateStream(stream, CompressionMode.Decompress)) {
				reader = new BinaryReader(gs);
				if (reader.ReadByte() != (byte)NbtTagType.Compound)
					throw new InvalidDataException("Nbt file must start with Tag_Compound");
				this.game = game;
				map = game.World;
				file = new NbtFile(reader);
				
				NbtTag root = file.ReadTag((byte)NbtTagType.Compound, true);
				NbtCompound children = (NbtCompound)root.Value;
				if (children.ContainsKey("Metadata"))
					ParseMetadata(children);
				
				NbtCompound spawn = (NbtCompound)children["Spawn"].Value;
				LocalPlayer p = game.LocalPlayer;
				p.Spawn.X = (short)spawn["X"].Value;
				p.Spawn.Y = (short)spawn["Y"].Value;
				p.Spawn.Z = (short)spawn["Z"].Value;
				if (spawn.ContainsKey("H"))
					p.SpawnYaw = (float)Utils.PackedToDegrees((byte)spawn["H"].Value);
				if (spawn.ContainsKey("P"))
					p.SpawnPitch = (float)Utils.PackedToDegrees((byte)spawn["P"].Value);
				
				map.Uuid = new Guid((byte[])children["UUID"].Value);
				width = (short)children["X"].Value;
				height = (short)children["Y"].Value;
				length = (short)children["Z"].Value;
				
				// Older versions incorrectly multiplied spawn coords by * 32, so we check for that.
				if (p.Spawn.X < 0 || p.Spawn.X >= width || p.Spawn.Y < 0 ||
				   p.Spawn.Y >= height || p.Spawn.Z < 0 || p.Spawn.Z >= length) {
					p.Spawn.X /= 32; p.Spawn.Y /= 32; p.Spawn.Z /= 32;
				}
				return (byte[])children["BlockArray"].Value;
			}
		}
		
		void ParseMetadata(NbtCompound children) {
			NbtCompound metadata = (NbtCompound)children["Metadata"].Value;
			NbtTag cpeTag;
			LocalPlayer p = game.LocalPlayer;
			if (!metadata.TryGetValue("CPE", out cpeTag)) return;
			
			metadata = (NbtCompound)cpeTag.Value;
			if (CheckKey("ClickDistance", 1, metadata)) {
				p.ReachDistance = (short)curCpeExt["Distance"].Value / 32f;
			}
			if (CheckKey("EnvColors", 1, metadata)) {
				map.Env.SetSkyColour(GetColour("Sky", WorldEnv.DefaultSkyColour));
				map.Env.SetCloudsColour(GetColour("Cloud", WorldEnv.DefaultCloudsColour));
				map.Env.SetFogColour(GetColour("Fog", WorldEnv.DefaultFogColour));
				map.Env.SetSunlight(GetColour("Sunlight", WorldEnv.DefaultSunlight));
				map.Env.SetShadowlight(GetColour("Ambient", WorldEnv.DefaultShadowlight));
			}
			if (CheckKey("EnvMapAppearance", 1, metadata)) {
				string url = null;
				if (curCpeExt.ContainsKey("TextureURL"))
					url = (string)curCpeExt["TextureURL"].Value;
				if (url.Length == 0) url = null;
				if (game.AllowServerTextures && url != null)
					game.Server.RetrieveTexturePack(url);
				
				byte sidesBlock = (byte)curCpeExt["SideBlock"].Value;
				byte edgeBlock = (byte)curCpeExt["EdgeBlock"].Value;
				map.Env.SetSidesBlock(sidesBlock);
				map.Env.SetEdgeBlock(edgeBlock);
				map.Env.SetEdgeLevel((short)curCpeExt["SideLevel"].Value);
			}
			if (CheckKey("EnvWeatherType", 1, metadata)) {
				byte weather = (byte)curCpeExt["WeatherType"].Value;
				map.Env.SetWeather((Weather)weather);
			}
			
			if (game.AllowCustomBlocks && CheckKey("BlockDefinitions", 1, metadata)) {
				foreach (var pair in curCpeExt) {
					if (pair.Value.TagId != NbtTagType.Compound) continue;
					if (!Utils.CaselessStarts(pair.Key, "Block")) continue;
					ParseBlockDefinition((NbtCompound)pair.Value.Value);
				}
			}
		}
		
		NbtCompound curCpeExt;
		bool CheckKey(string key, int version, NbtCompound tag) {
			NbtTag value;
			if (!tag.TryGetValue(key, out value)) return false;
			if (value.TagId != NbtTagType.Compound) return false;
			
			tag = (NbtCompound)value.Value;
			if (!tag.TryGetValue("ExtensionVersion", out value)) return false;
			if ((int)value.Value == version) {
				curCpeExt = tag;
				return true;
			}
			return false;
		}
		
		FastColour GetColour(string key, FastColour def) {
			NbtTag tag;
			if (!curCpeExt.TryGetValue(key, out tag))
				return def;
			
			NbtCompound compound = (NbtCompound)tag.Value;
			short r = (short)compound["R"].Value;
			short g = (short)compound["G"].Value;
			short b = (short)compound["B"].Value;
			bool invalid = r < 0 || r > 255 || g < 0 || g > 255 || b < 0 || b > 255;
			return invalid ? def : new FastColour(r, g, b);
		}
		
		void ParseBlockDefinition(NbtCompound compound) {
			byte id = (byte)compound["ID"].Value;
			BlockInfo info = game.BlockInfo;
			info.Name[id] = (string)compound["Name"].Value;
			info.Collide[id] = (CollideType)compound["CollideType"].Value;
			info.SpeedMultiplier[id] = (float)compound["Speed"].Value;
			
			byte[] data = (byte[])compound["Textures"].Value;
			info.SetTex(data[0], Side.Top, id);
			info.SetTex(data[1], Side.Bottom, id);
			info.SetTex(data[2], Side.Left, id);
			info.SetTex(data[3], Side.Right, id);
			info.SetTex(data[4], Side.Front, id);
			info.SetTex(data[5], Side.Back, id);
			
			info.BlocksLight[id] = (byte)compound["TransmitsLight"].Value == 0;
			byte soundId = (byte)compound["WalkSound"].Value;
			info.DigSounds[id] = CPEProtocolBlockDefs.breakSnds[soundId];
			info.StepSounds[id] = CPEProtocolBlockDefs.stepSnds[soundId];
			info.FullBright[id] = (byte)compound["FullBright"].Value != 0;
			
			byte blockDraw = (byte)compound["BlockDraw"].Value;
			if ((byte)compound["Shape"].Value == 0)
				blockDraw = DrawType.Sprite;
			info.SetBlockDraw(id, blockDraw);
			
			data = (byte[])compound["Fog"].Value;
			info.FogDensity[id] = (data[0] + 1) / 128f;
			// Fix for older ClassicalSharp versions which saved wrong fog density value
			if (data[0] == 0xFF) info.FogDensity[id] = 0;
			info.FogColour[id] = new FastColour(data[1], data[2], data[3]);

			data = (byte[])compound["Coords"].Value;
			info.MinBB[id] = new Vector3(data[0] / 16f, data[1] / 16f, data[2] / 16f);
			info.MaxBB[id] = new Vector3(data[3] / 16f, data[4] / 16f, data[5] / 16f);
			
			info.UpdateCulling(id);
			info.LightOffset[id] = info.CalcLightOffset(id);
			game.Events.RaiseBlockDefinitionChanged();
			info.DefinedCustomBlocks[id >> 5] |= (1u << (id & 0x1F));
			
			game.Inventory.CanPlace.SetNotOverridable(true, id);
			game.Inventory.CanDelete.SetNotOverridable(true, id);
			game.Events.RaiseBlockPermissionsChanged();
		}
	}
}