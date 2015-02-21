using System;
using System.IO;
using ClassicalSharp.Entities;
using ClassicalSharp.Util;
using ClassicalSharp.Window;
using ClassicalSharp.World;
using Ionic.Zlib;
using OpenTK;

namespace ClassicalSharp.Network.Packets {
	
	public sealed class KeepAliveInbound : InboundPacket {
		
		public override void ReadData( NetReader reader ) {
		}
		
		public override void ReadCallback( Game game ) {
			game.Network.SendPacket( new KeepAliveOutbound() );
		}
	}
	
	public sealed class LoginRequestInbound : InboundPacket {
		int entityId;
		string unknown;
		long mapSeed;
		byte dimension;
		
		public override void ReadData( NetReader reader ) {
			entityId = reader.ReadInt32();
			unknown = reader.ReadString();
			mapSeed = reader.ReadInt64();
			dimension = reader.ReadUInt8();
		}
		
		public override void ReadCallback( Game game ) {
			game.EntityManager[entityId] = game.LocalPlayer;
			game.Map.Seed = mapSeed;
			game.Map.Dimension = dimension;
			game.RaiseOnNewMap();
		}
	}
	
	public sealed class HandshakeInbound : InboundPacket {
		string hash;
		
		public override void ReadData( NetReader reader ) {
			hash = reader.ReadString();
		}
		
		public override void ReadCallback( Game game ) {
			if( hash != "-" ) {
				game.SetNewScreen( new ErrorScreen( game, "Lost connection to server", "Cannot connect to online mode server" ) );
				game.Network.Dispose();
			} else {
				game.Network.SendPacket( new LoginRequestOutbound( game.Username ) );
			}
		}
	}
	
	public sealed class ChatInbound : InboundPacket {
		string text;
		
		public override void ReadData( NetReader reader ) {
			text = reader.ReadString();
		}
		
		public override void ReadCallback( Game game ) {
			game.AddChat( text );
		}
	}
	
	public sealed class TimeUpdateInbound : InboundPacket {
		long worldAge;
		
		public override void ReadData( NetReader reader ) {
			worldAge = reader.ReadInt64();
		}
		
		public override void ReadCallback( Game game ) {
			game.Map.WorldTime = worldAge;
		}
	}
	
	public sealed class EntityEquipmentInbound : InboundPacket {
		int entityId;
		short slotId;
		Slot slot;
		
		public override void ReadData( NetReader reader ) {
			entityId = reader.ReadInt32();
			slotId = reader.ReadInt16();
			short id = reader.ReadInt16();
			short damage = reader.ReadInt16();
			slot = new Slot() { Id = id, Count = 1, Damage = damage };
		}
		
		public override void ReadCallback( Game game ) {
			game.EntityManager[entityId].SetEquipmentSlot( slotId, slot );
		}
	}
	
	public sealed class SpawnPositionInbound : InboundPacket {
		Vector3I spawnPos;
		
		public override void ReadData( NetReader reader ) {
			spawnPos.X = reader.ReadInt32();
			spawnPos.Y = reader.ReadInt32();
			spawnPos.Z = reader.ReadInt32();
		}
		
		public override void ReadCallback( Game game ) {
			game.LocalPlayer.SpawnPoint = (Vector3)spawnPos;
		}
	}
	
	public sealed class UpdateHealthInbound : InboundPacket {
		short health;
		
		public override void ReadData( NetReader reader ) {
			health = reader.ReadInt16();
		}
		
		public override void ReadCallback( Game game ) {
			game.LocalPlayer.Health = health;
		}
	}
	
	public sealed class RespawnInbound : InboundPacket {
		byte dimension;
		
		public override void ReadData( NetReader reader ) {
			dimension = reader.ReadUInt8();
		}
		
		public override void ReadCallback( Game game ) {
			throw new NotImplementedException();
		}
	}
	
	public sealed class PlayerPosAndLookInbound : InboundPacket {
		float x, y, z, stanceY;
		float yaw, pitch;
		
		public override void ReadData( NetReader reader ) {
			x = (float)reader.ReadFloat64();
			stanceY = (float)reader.ReadFloat64();
			y = (float)reader.ReadFloat64();
			z = (float)reader.ReadFloat64();
			yaw = reader.ReadFloat32();
			pitch = reader.ReadFloat32();
		}
		
		public override void ReadCallback( Game game ) {
			LocationUpdate update = LocationUpdate.MakePosAndOri( x, y, z, yaw, pitch, false );
			game.Map.IsNotLoaded = false;
			if( !game.Network.receivedFirstPosition ) {
				game.RaiseOnNewMapLoaded();
			}
			game.Network.receivedFirstPosition = true;
			game.SetNewScreen( new NormalScreen( game ) );		
			game.LocalPlayer.SetLocation( update, false );
		}
	}
	
	public sealed class UseBedInbound : InboundPacket {
		int entityId;
		byte unknown;
		int x, y, z;
		
		public override void ReadData( NetReader reader ) {
			entityId = reader.ReadInt32();
			unknown = reader.ReadUInt8();
			x = reader.ReadInt32();
			y = reader.ReadUInt8();
			z = reader.ReadInt32();
		}
		
		public override void ReadCallback( Game game ) {
			throw new NotImplementedException();
		}
	}
	
	public sealed class AnimationInbound : InboundPacket {
		int entityId;
		byte anim;
		
		public override void ReadData( NetReader reader ) {
			entityId = reader.ReadInt32();
			anim = reader.ReadUInt8();
		}
		
		public override void ReadCallback( Game game ) {
			game.EntityManager[entityId].DoAnimation( anim );
		}
	}
	
	public sealed class SpawnPlayerInbound : InboundPacket {
		int entityId;
		string playerName;
		float x, y, z;
		float yaw, pitch;
		short currentItem;
		
		public override void ReadData( NetReader reader ) {
			entityId = reader.ReadInt32();
			playerName = reader.ReadString();
			x = reader.ReadInt32() / 32f;
			y = reader.ReadInt32() / 32f;
			z = reader.ReadInt32() / 32f;
			yaw = (float)Utils.PackedToDegrees( reader.ReadUInt8() );
			pitch = (float)Utils.PackedToDegrees( reader.ReadUInt8() );
			currentItem = reader.ReadInt16();
		}
		
		public override void ReadCallback( Game game ) {
			NetPlayer player = new NetPlayer( playerName, game );
			LocationUpdate update = LocationUpdate.MakePosAndOri( x, y, z, yaw, pitch, false );
			player.SetLocation( update, false );
			player.CurrentItem = currentItem;
			game.EntityManager[entityId] = player;
		}
	}
	
	public sealed class SpawnPickupInbound : InboundPacket {
		int entityId;
		short itemId;
		byte itemsCount;
		short itemDamage;
		float x, y, z;
		float yaw, pitch, roll;
		
		public override void ReadData( NetReader reader ) {
			entityId = reader.ReadInt32();
			itemId = reader.ReadInt16();
			itemsCount = reader.ReadUInt8();
			itemDamage = reader.ReadInt16();
			x = reader.ReadInt32() / 32f;
			y = reader.ReadInt32() / 32f;
			z = reader.ReadInt32() / 32f;
			yaw = (float)Utils.PackedToDegrees( reader.ReadUInt8() );
			pitch = (float)Utils.PackedToDegrees( reader.ReadUInt8() );
			roll = (float)Utils.PackedToDegrees( reader.ReadUInt8() );
		}
		
		public override void ReadCallback( Game game ) {
			Entity entity = new NullEntity( game );
			LocationUpdate update = LocationUpdate.MakePosAndOri( x, y, z, yaw, pitch, false );
			entity.SetLocation( update, false );
			game.EntityManager[entityId] = entity;
			throw new NotImplementedException();
		}
	}
	
	public sealed class CollectItemInbound : InboundPacket {
		int collectedId;
		int collectorId;
		
		public override void ReadData( NetReader reader ) {
			collectedId = reader.ReadInt32();
			collectorId = reader.ReadInt32();
		}
		
		public override void ReadCallback( Game game ) {
			throw new NotImplementedException();
		}
	}
	
	public sealed class SpawnObjectInbound : InboundPacket {
		int entityId;
		byte type;
		float x, y, z;
		int objData;
		float velX, velY, velZ;
		
		public override void ReadData( NetReader reader ) {
			entityId = reader.ReadInt32();
			type = reader.ReadUInt8();
			x = reader.ReadInt32() / 32f;
			y = reader.ReadInt32() / 32f;
			z = reader.ReadInt32() / 32f;
			objData = reader.ReadInt32();
			if( objData > 0 ) {
				velX = reader.ReadInt16() / 8000f;
				velY = reader.ReadInt16() / 8000f;
				velZ = reader.ReadInt16() / 8000f;
			}
		}
		
		public override void ReadCallback( Game game ) {
			Entity entity = new NullEntity( game );
			LocationUpdate update = LocationUpdate.MakePos( x, y, z, false );
			entity.SetLocation( update, false );
			entity.SetVelocity( velX, velY, velZ );
			game.EntityManager[entityId] = entity;
			throw new NotImplementedException();
		}
	}
	
	public sealed class SpawnMobInbound : InboundPacket {
		int entityId;
		byte type;
		float x, y, z;
		float yaw, pitch;
		EntityMetadata meta;
		
		public override void ReadData( NetReader reader ) {
			entityId = reader.ReadInt32();
			type = reader.ReadUInt8();
			x = reader.ReadInt32() / 32f;
			y = reader.ReadInt32() / 32f;
			z = reader.ReadInt32() / 32f;
			yaw = (float)Utils.PackedToDegrees( reader.ReadUInt8() );
			pitch = (float)Utils.PackedToDegrees( reader.ReadUInt8() );
			meta = EntityMetadata.ReadFrom( reader );
		}
		
		public override void ReadCallback( Game game ) {
			Entity entity = new NullEntity( game );
			LocationUpdate update = LocationUpdate.MakePosAndOri( x, y, z, yaw, pitch, false );
			entity.SetLocation( update, false );
			entity.SetMetadata( meta );
			game.EntityManager[entityId] = entity;
			throw new NotImplementedException();
		}
	}
	
	public sealed class SpawnPaintingInbound : InboundPacket {
		int entityId;
		string title;
		int x, y, z;
		int direction;
		
		public override void ReadData( NetReader reader ) {
			entityId = reader.ReadInt32();
			title = reader.ReadString();
			x = reader.ReadInt32();
			y = reader.ReadInt32();
			z = reader.ReadInt32();
			direction = reader.ReadInt32();
		}
		
		public override void ReadCallback( Game game ) {
			game.EntityManager[entityId] = new NullEntity( game );
			throw new NotImplementedException();
		}
	}
	
	public sealed class EntityVelocityInbound : InboundPacket {
		int entityId;
		float velX, velY, velZ;
		
		public override void ReadData( NetReader reader ) {
			entityId = reader.ReadInt32();
			velX = reader.ReadInt16() / 8000f;
			velY = reader.ReadInt16() / 8000f;
			velZ = reader.ReadInt16() / 8000f;
		}
		
		public override void ReadCallback( Game game ) {
			game.EntityManager[entityId].SetVelocity( velX, velY, velZ );
		}
	}
	
	public sealed class DestroyEntityInbound : InboundPacket {
		int entityId;
		
		public override void ReadData( NetReader reader ) {
			entityId = reader.ReadInt32();
		}
		
		public override void ReadCallback( Game game ) {
			game.EntityManager.RemoveEntity( entityId );
		}
	}
	
	public sealed class EntityInbound : InboundPacket {
		int entityId;
		
		public override void ReadData( NetReader reader ) {
			entityId = reader.ReadInt32();
		}
		
		public override void ReadCallback( Game game ) {
			// Pointless packet.. do nothing.
		}
	}
	
	public sealed class EntityRelativeMoveInbound : InboundPacket {
		int entityId;
		float dx, dy, dz;
		
		public override void ReadData( NetReader reader ) {
			entityId = reader.ReadInt32();
			dx = reader.ReadInt8() / 32f;
			dy = reader.ReadInt8() / 32f;
			dz = reader.ReadInt8() / 32f;
		}
		
		public override void ReadCallback( Game game ) {
			LocationUpdate update = LocationUpdate.MakePos( dx, dy, dz, true );
			game.EntityManager[entityId].SetLocation( update, true );
		}
	}
	
	public sealed class EntityLookInbound : InboundPacket {
		int entityId;
		float yaw, pitch;
		
		public override void ReadData( NetReader reader ) {
			entityId = reader.ReadInt32();
			yaw = (float)Utils.PackedToDegrees( reader.ReadUInt8() );
			pitch = (float)Utils.PackedToDegrees( reader.ReadUInt8() );
		}
		
		public override void ReadCallback( Game game ) {
			LocationUpdate update = LocationUpdate.MakeOri( yaw, pitch );
			game.EntityManager[entityId].SetLocation( update, true );
		}
	}
	
	public sealed class EntityLookAndRelativeMoveInbound : InboundPacket {
		int entityId;
		float dx, dy, dz;
		float yaw, pitch;
		
		public override void ReadData( NetReader reader ) {
			entityId = reader.ReadInt32();
			dx = reader.ReadInt8() / 32f;
			dy = reader.ReadInt8() / 32f;
			dz = reader.ReadInt8() / 32f;
			yaw = (float)Utils.PackedToDegrees( reader.ReadUInt8() );
			pitch = (float)Utils.PackedToDegrees( reader.ReadUInt8() );
		}
		
		public override void ReadCallback( Game game ) {
			LocationUpdate update = LocationUpdate.MakePosAndOri( dx, dy, dz, yaw, pitch, true );
			game.EntityManager[entityId].SetLocation( update, true );
		}
	}
	
	public sealed class EntityTeleportInbound : InboundPacket {
		int entityId;
		float x, y, z;
		float yaw, pitch;
		
		public override void ReadData( NetReader reader ) {
			entityId = reader.ReadInt32();
			x = reader.ReadInt32() / 32f;
			y = reader.ReadInt32() / 32f;
			z = reader.ReadInt32() / 32f;
			yaw = (float)Utils.PackedToDegrees( reader.ReadUInt8() );
			pitch = (float)Utils.PackedToDegrees( reader.ReadUInt8() );
		}
		
		public override void ReadCallback( Game game ) {
			LocationUpdate update = LocationUpdate.MakePosAndOri( x, y, z, yaw, pitch, false );
			game.EntityManager[entityId].SetLocation( update, true );
		}
	}
	
	public sealed class EntityStatusInbound : InboundPacket {
		int entityId;
		byte status;
		
		public override void ReadData( NetReader reader ) {
			entityId = reader.ReadInt32();
			status = reader.ReadUInt8();
		}
		
		public override void ReadCallback( Game game ) {
			game.EntityManager[entityId].SetStatus( status );
		}
	}
	
	public sealed class AttachEntityInbound : InboundPacket {
		int collectedId;
		int collectorId;
		
		public override void ReadData( NetReader reader ) {
			collectedId = reader.ReadInt32();
			collectorId = reader.ReadInt32();
		}
		
		public override void ReadCallback( Game game ) {
			throw new NotImplementedException();
		}
	}
	
	public sealed class EntityMetadataInbound : InboundPacket {
		int entityId;
		EntityMetadata meta;
		
		public override void ReadData( NetReader reader ) {
			entityId = reader.ReadInt32();
			meta = EntityMetadata.ReadFrom( reader );
		}
		
		public override void ReadCallback( Game game ) {
			game.EntityManager[entityId].SetMetadata( meta );
		}
	}
	
	public sealed class PrepareChunkInbound : InboundPacket {
		int chunkX, chunkZ;
		bool load;
		
		public override void ReadData( NetReader reader ) {
			chunkX = reader.ReadInt32();
			chunkZ = reader.ReadInt32();
			load = reader.ReadBool();
		}
		
		public override void ReadCallback( Game game ) {
			if( !load ) {
				game.Map.UnloadChunk( chunkX, chunkZ );
			}
		}
	}
	
	public sealed class MapChunkInbound : InboundPacket {
		int x, y, z;
		int sizeX, sizeY, sizeZ;
		byte[] compressedData;
		
		public override void ReadData( NetReader reader ) {
			x = reader.ReadInt32();
			y = reader.ReadInt16();
			z = reader.ReadInt32();
			sizeX = reader.ReadUInt8() + 1;
			sizeY = reader.ReadUInt8() + 1;
			sizeZ = reader.ReadUInt8() + 1;
			int compressedSize = reader.ReadInt32();
			compressedData = reader.ReadRawBytes( compressedSize );
		}
		
		public override void ReadCallback( Game game ) {
			int elementSize = sizeX * sizeY * sizeZ;
			int totalSize = elementSize + elementSize * 3 / 2; // blocks + meta + blocklight + skylight
			byte[] data = Decompress( compressedData, totalSize );

			if( sizeX == 16 && sizeY == Map.Height && sizeZ == 16 ) { // Full chunk
				Chunk chunk = new Chunk( x >> 4, z >> 4, false, game );
				int index = 0;
				chunk.Blocks = GetPortion( data, elementSize, ref index );
				chunk.Metadata = new NibbleArray( GetPortion( data, elementSize / 2, ref index ) );
				chunk.BlockLight = new NibbleArray( GetPortion( data, elementSize / 2, ref index ) );
				chunk.SkyLight = new NibbleArray( GetPortion( data, elementSize / 2, ref index ) );
				game.Map.UnloadChunk( chunk.ChunkX, chunk.ChunkZ );
				game.Map.LoadChunk( chunk );
			} else {
				if( sizeX > 16 || sizeZ > 16 ) {
					Utils.LogWarning( "Cannot update multiple chunks at once yet." );
					return;
				}
				Chunk chunk = game.Map.GetChunk( x >> 4, z >> 4 );
				if( chunk == null ) {
					chunk = new Chunk( x >> 4, z >> 4, true, game );
					game.Map.AddChunk( chunk );
				}
				int startX = x & 0x0F;
				int startY = y;
				int startZ = z & 0x0F;
				
				int index = 0;
				byte[] blocks = GetPortion( data, elementSize, ref index );
				NibbleArray metadata = new NibbleArray( GetPortion( data, elementSize / 2, ref index ) );
				ChunkPartialUpdate[] updates = new ChunkPartialUpdate[Map.Height / 16];
				
				for( int i = 0; i < elementSize; i++ ) {
					int blockX = startX + ( ( i / sizeY ) / sizeZ );
					int blockY = startY + ( i % sizeY );
					int blockZ = startZ + ( ( i / sizeY ) % sizeZ );
					updates[blockY >> 4].UpdateModifiedState( blockX, blockY, blockZ );
					chunk.SetBlock( blockX, blockY, blockZ, blocks[i] );
					chunk.SetBlockMetadata( blockX, blockY, blockZ, metadata[i] );
				}
				game.NotifyChunkPartialUpdate( chunk, updates );
			}
		}
		
		static byte[] GetPortion( byte[] data, int length, ref int index ) {
			byte[] portion = new byte[length];
			Buffer.BlockCopy( data, index, portion, 0, length );
			index += length;
			return portion;
		}
		
		static byte[] Decompress( byte[] compressed, int uncompressedSize ) {
			byte[] temp = new byte[1024];
			
			using( MemoryStream dest = new MemoryStream( uncompressedSize ) ) {
				using( MemoryStream src = new MemoryStream( compressed ) ) {
					using( Stream decompressor = new ZlibStream( src, CompressionMode.Decompress ) ) {
						int count;
						while( ( count = decompressor.Read( temp, 0, temp.Length ) ) != 0 ) {
							dest.Write( temp, 0, count );
						}
						if( dest.Capacity != dest.Length ) throw new InvalidOperationException( "uncompressed size wrong?" );
						return dest.GetBuffer();
					}
				}
			}
		}
	}
	
	public sealed class MultiBlockChangeInbound : InboundPacket {
		int chunkX, chunkZ;
		ushort[] posMasks;
		byte[] blockTypes;
		byte[] blockMetas;
		
		public override void ReadData( NetReader reader ) {
			chunkX = reader.ReadInt32();
			chunkZ = reader.ReadInt32();
			int count = reader.ReadInt16();
			posMasks = new ushort[count];
			for( int i = 0; i < count; i++ ) {
				posMasks[i] = reader.ReadUInt16();
			}
			blockTypes = reader.ReadRawBytes( count );
			blockMetas = reader.ReadRawBytes( count );
		}
		
		public override void ReadCallback( Game game ) {
			Chunk chunk = game.Map.GetChunk( chunkX, chunkZ );
			if( chunk != null ) {
				ChunkPartialUpdate[] updates = new ChunkPartialUpdate[Map.Height / 16];
				
				for( int i = 0; i < posMasks.Length; i++ ) {
					int mask = posMasks[i];
					int y = mask & 0x00FF;
					int z = ( mask & 0x0F00 ) >> 8;
					int x = ( mask & 0xF000 ) >> 12;
					updates[y >> 4].UpdateModifiedState( x, y, z );
					chunk.SetBlock( x, y, z, blockTypes[i] );
					chunk.SetBlockMetadata( x, y, z, blockMetas[i] );
				}
				game.NotifyChunkPartialUpdate( chunk, updates );
			}
		}
	}
	
	public sealed class BlockChangeInbound : InboundPacket {
		int x, y, z;
		byte blockType, blockMeta;
		
		public override void ReadData( NetReader reader ) {
			x = reader.ReadInt32();
			y = reader.ReadUInt8();
			z = reader.ReadInt32();
			blockType = reader.ReadUInt8();
			blockMeta = reader.ReadUInt8();
		}
		
		public override void ReadCallback( Game game ) {
			game.UpdateBlock( x, y, z, blockType, blockMeta );
		}
	}
	
	public sealed class BlockActionInbound : InboundPacket {
		int x, y, z;
		byte data1, data2;
		
		public override void ReadData( NetReader reader ) {
			x = reader.ReadInt32();
			y = reader.ReadInt16();
			z = reader.ReadInt32();
			data1 = reader.ReadUInt8();
			data2 = reader.ReadUInt8();
		}
		
		public override void ReadCallback( Game game ) {
			throw new NotImplementedException();
		}
	}
	
	public sealed class ExplosionInbound : InboundPacket {
		float x, y, z;
		float radius;
		Record[] records;
		
		struct Record {
			public sbyte X, Y, Z;
			
			public Record( sbyte x, sbyte y, sbyte z ) {
				X = x;
				Y = y;
				Z = z;
			}
		}
		
		public override void ReadData( NetReader reader ) {
			x = (float)reader.ReadFloat64();
			y = (float)reader.ReadFloat64();
			z = (float)reader.ReadFloat64();
			radius = reader.ReadFloat32();
			records = new Record[reader.ReadInt32()];
			for( int i = 0; i < records.Length; i++ ) {
				records[i] = new Record( reader.ReadInt8(), reader.ReadInt8(), reader.ReadInt8() );
			}
		}
		
		public override void ReadCallback( Game game ) {
			throw new NotImplementedException();
		}
	}
	
	public sealed class SoundEffectInbound : InboundPacket {
		int effectId;
		int x, y, z;
		int data;
		
		public override void ReadData( NetReader reader ) {
			effectId = reader.ReadInt32();
			x = reader.ReadInt32();
			y = reader.ReadInt8();
			z = reader.ReadInt32();
			data = reader.ReadInt32();
		}
		
		public override void ReadCallback( Game game ) {
			throw new NotImplementedException();
		}
	}
	
	public sealed class ChangeGameStateInbound : InboundPacket {
		byte state;
		
		public override void ReadData( NetReader reader ) {
			state = reader.ReadUInt8();
		}
		
		public override void ReadCallback( Game game ) {
			throw new NotImplementedException();
		}
	}
	
	public sealed class ThunderboltInbound : InboundPacket {
		int entityId;
		byte unknown;
		float x, y, z;
		
		public override void ReadData( NetReader reader ) {
			entityId = reader.ReadInt32();
			unknown = reader.ReadUInt8();
			x = reader.ReadInt32() / 32f;
			y = reader.ReadInt32() / 32f;
			z = reader.ReadInt32() / 32f;
		}
		
		public override void ReadCallback( Game game ) {
			throw new NotImplementedException();
		}
	}
	
	public sealed class OpenWindowInbound : InboundPacket {
		byte windowId;
		string inventoryType;
		string windowTitle;
		byte numSlots;
		
		public override void ReadData( NetReader reader ) {
			windowId = reader.ReadUInt8();
			inventoryType = reader.ReadString();
			windowTitle = reader.ReadString();
			numSlots = reader.ReadUInt8();
		}
		
		public override void ReadCallback( Game game ) {
			throw new NotImplementedException();
		}
	}
	
	public sealed class CloseWindowInbound : InboundPacket {
		byte windowId;
		
		public override void ReadData( NetReader reader ) {
			windowId = reader.ReadUInt8();
		}
		
		public override void ReadCallback( Game game ) {
			throw new NotImplementedException();
		}
	}
	
	public sealed class SetSlotInbound : InboundPacket {
		byte windowId;
		short slotIndex;
		Slot slot;
		
		public override void ReadData( NetReader reader ) {
			windowId = reader.ReadUInt8();
			slotIndex = reader.ReadInt16();
			slot = reader.ReadSlot();
		}
		
		public override void ReadCallback( Game game ) {
			game.WindowManager.SetSlot( windowId, slotIndex, slot );
		}
	}
	
	public sealed class WindowItemsInbound : InboundPacket {
		byte windowId;
		Slot[] slots;
		
		public override void ReadData( NetReader reader ) {
			windowId = reader.ReadUInt8();
			slots = new Slot[reader.ReadInt16()];
			for( int i = 0; i < slots.Length; i++ ) {
				slots[i] = reader.ReadSlot();
			}
		}
		
		public override void ReadCallback( Game game ) {
			game.WindowManager.SetAllSlots( windowId, slots );
		}
	}
	
	public sealed class WindowPropertyInbound : InboundPacket {
		byte windowId;
		short property;
		short value;
		
		public override void ReadData( NetReader reader ) {
			windowId = reader.ReadUInt8();
			property = reader.ReadInt16();
			value = reader.ReadInt16();
		}
		
		public override void ReadCallback( Game game ) {
			game.WindowManager.SetProperty( windowId, property, value );
		}
	}
	
	public sealed class ConfirmTransactionInbound : InboundPacket {
		byte windowId;
		short actionNum;
		bool accepted;
		
		public override void ReadData( NetReader reader ) {
			windowId = reader.ReadUInt8();
			actionNum = reader.ReadInt16();
			accepted = reader.ReadBool();
		}
		
		public override void ReadCallback( Game game ) {
			throw new NotImplementedException();
		}
	}
	
	public sealed class UpdateSignInbound : InboundPacket {
		int x, y, z;
		string line1, line2, line3, line4;
		
		public override void ReadData( NetReader reader ) {
			x = reader.ReadInt32();
			y = reader.ReadInt16();
			z = reader.ReadInt32();
			line1 = reader.ReadString();
			line2 = reader.ReadString();
			line3 = reader.ReadString();
			line4 = reader.ReadString();
		}
		
		public override void ReadCallback( Game game ) {
			throw new NotImplementedException();
		}
	}
	
	public sealed class MapsInbound : InboundPacket {
		short itemType;
		short itemDamage;
		byte[] data;
		
		public override void ReadData( NetReader reader ) {
			itemType = reader.ReadInt16();
			itemDamage = reader.ReadInt16();
			data = reader.ReadRawBytes( reader.ReadUInt8() );
		}
		
		public override void ReadCallback( Game game ) {
			throw new NotImplementedException();
		}
	}
	
	public sealed class StatisticsInbound : InboundPacket {
		int statId;
		byte amount;
		
		public override void ReadData( NetReader reader ) {
			statId = reader.ReadInt32();
			amount = reader.ReadUInt8();
		}
		
		public override void ReadCallback( Game game ) {
			throw new NotImplementedException();
		}
	}
	
	public sealed class DisconnectInbound : InboundPacket {
		string reason;
		
		public override void ReadData( NetReader reader ) {
			reason = reader.ReadString();
		}
		
		public override void ReadCallback( Game game ) {
			game.SetNewScreen( new ErrorScreen( game, "Lost connection to server", reason ) );
			game.Network.Dispose();
		}
	}
}
