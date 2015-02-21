using System;

namespace Injector {
	
	public abstract class PacketS2C {
		
		public abstract void ReadData( StreamInjector reader );
	}
	
	public sealed class KeepAliveInbound : PacketS2C {
		
		public override void ReadData( StreamInjector reader ) {
		}
	}
	
	public sealed class LoginRequestInbound : PacketS2C {
		int entityId;
		string unknown;
		long mapSeed;
		byte dimension;
		
		public override void ReadData( StreamInjector reader ) {
			entityId = reader.ReadInt32();
			unknown = reader.ReadString();
			mapSeed = reader.ReadInt64();
			dimension = reader.ReadUInt8();
		}
	}
	
	public sealed class HandshakeInbound : PacketS2C {
		string hash;
		
		public override void ReadData( StreamInjector reader ) {
			hash = reader.ReadString();
		}
	}
	
	public sealed class ChatInbound : PacketS2C {
		string text;
		
		public override void ReadData( StreamInjector reader ) {
			text = reader.ReadString();
		}
	}
	
	public sealed class TimeUpdateInbound : PacketS2C {
		long worldAge;
		
		public override void ReadData( StreamInjector reader ) {
			worldAge = reader.ReadInt64();
		}
	}
	
	public sealed class EntityEquipmentInbound : PacketS2C {
		int entityId;
		short slotId;
		Slot slot;
		
		public override void ReadData( StreamInjector reader ) {
			entityId = reader.ReadInt32();
			slotId = reader.ReadInt16();
			short id = reader.ReadInt16();
			short damage = reader.ReadInt16();
			slot = new Slot() { Id = id, Count = 1, Damage = damage };
		}
	}
	
	public sealed class SpawnPositionInbound : PacketS2C {
		int spawnX, spawnY, spawnZ;
		
		public override void ReadData( StreamInjector reader ) {
			spawnX = reader.ReadInt32();
			spawnY = reader.ReadInt32();
			spawnZ = reader.ReadInt32();
		}
	}
	
	public sealed class UpdateHealthInbound : PacketS2C {
		short health;
		
		public override void ReadData( StreamInjector reader ) {
			health = reader.ReadInt16();
		}
	}
	
	public sealed class RespawnInbound : PacketS2C {
		byte dimension;
		
		public override void ReadData( StreamInjector reader ) {
			dimension = reader.ReadUInt8();
		}
	}
	
	public sealed class PlayerPosAndLookInbound : PacketS2C {
		float x, y, z, stanceY;
		float yaw, pitch;
		
		public override void ReadData( StreamInjector reader ) {
			x = (float)reader.ReadFloat64();
			stanceY = (float)reader.ReadFloat64();
			y = (float)reader.ReadFloat64();
			z = (float)reader.ReadFloat64();
			yaw = reader.ReadFloat32();
			pitch = reader.ReadFloat32();
		}
	}
	
	public sealed class UseBedInbound : PacketS2C {
		int entityId;
		byte unknown;
		int x, y, z;
		
		public override void ReadData( StreamInjector reader ) {
			entityId = reader.ReadInt32();
			unknown = reader.ReadUInt8();
			x = reader.ReadInt32();
			y = reader.ReadUInt8();
			z = reader.ReadInt32();
		}
	}
	
	public sealed class AnimationInbound : PacketS2C {
		int entityId;
		byte anim;
		
		public override void ReadData( StreamInjector reader ) {
			entityId = reader.ReadInt32();
			anim = reader.ReadUInt8();
		}
	}
	
	public sealed class SpawnPlayerInbound : PacketS2C {
		int entityId;
		string playerName;
		float x, y, z;
		float yaw, pitch;
		short currentItem;
		
		public override void ReadData( StreamInjector reader ) {
			entityId = reader.ReadInt32();
			playerName = reader.ReadString();
			x = reader.ReadInt32() / 32f;
			y = reader.ReadInt32() / 32f;
			z = reader.ReadInt32() / 32f;
			yaw = (float)Utils.PackedToDegrees( reader.ReadUInt8() );
			pitch = (float)Utils.PackedToDegrees( reader.ReadUInt8() );
			currentItem = reader.ReadInt16();
		}
	}
	
	public sealed class SpawnPickupInbound : PacketS2C {
		int entityId;
		short itemId;
		byte itemsCount;
		short itemDamage;
		float x, y, z;
		float yaw, pitch, roll;
		
		public override void ReadData( StreamInjector reader ) {
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
	}
	
	public sealed class CollectItemInbound : PacketS2C {
		int collectedId;
		int collectorId;
		
		public override void ReadData( StreamInjector reader ) {
			collectedId = reader.ReadInt32();
			collectorId = reader.ReadInt32();
		}
	}
	
	public sealed class SpawnObjectInbound : PacketS2C {
		int entityId;
		byte type;
		float x, y, z;
		int objData;
		float velX, velY, velZ;
		
		public override void ReadData( StreamInjector reader ) {
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
	}
	
	public sealed class SpawnMobInbound : PacketS2C {
		int entityId;
		byte type;
		float x, y, z;
		float yaw, pitch;
		EntityMetadata meta;
		
		public override void ReadData( StreamInjector reader ) {
			entityId = reader.ReadInt32();
			type = reader.ReadUInt8();
			x = reader.ReadInt32() / 32f;
			y = reader.ReadInt32() / 32f;
			z = reader.ReadInt32() / 32f;
			yaw = (float)Utils.PackedToDegrees( reader.ReadUInt8() );
			pitch = (float)Utils.PackedToDegrees( reader.ReadUInt8() );
			meta = EntityMetadata.ReadFrom( reader );
		}
	}
	
	public sealed class SpawnPaintingInbound : PacketS2C {
		int entityId;
		string title;
		int x, y, z;
		int direction;
		
		public override void ReadData( StreamInjector reader ) {
			entityId = reader.ReadInt32();
			title = reader.ReadString();
			x = reader.ReadInt32();
			y = reader.ReadInt32();
			z = reader.ReadInt32();
			direction = reader.ReadInt32();
		}
	}
	
	public sealed class EntityVelocityInbound : PacketS2C {
		int entityId;
		float velX, velY, velZ;
		
		public override void ReadData( StreamInjector reader ) {
			entityId = reader.ReadInt32();
			velX = reader.ReadInt16() / 8000f;
			velY = reader.ReadInt16() / 8000f;
			velZ = reader.ReadInt16() / 8000f;
		}
	}
	
	public sealed class DestroyEntityInbound : PacketS2C {
		int entityId;
		
		public override void ReadData( StreamInjector reader ) {
			entityId = reader.ReadInt32();
		}
	}
	
	public sealed class EntityInbound : PacketS2C {
		int entityId;
		
		public override void ReadData( StreamInjector reader ) {
			entityId = reader.ReadInt32();
		}
	}
	
	public sealed class EntityRelativeMoveInbound : PacketS2C {
		int entityId;
		float dx, dy, dz;
		
		public override void ReadData( StreamInjector reader ) {
			entityId = reader.ReadInt32();
			dx = reader.ReadInt8() / 32f;
			dy = reader.ReadInt8() / 32f;
			dz = reader.ReadInt8() / 32f;
		}
	}
	
	public sealed class EntityLookInbound : PacketS2C {
		int entityId;
		float yaw, pitch;
		
		public override void ReadData( StreamInjector reader ) {
			entityId = reader.ReadInt32();
			yaw = (float)Utils.PackedToDegrees( reader.ReadUInt8() );
			pitch = (float)Utils.PackedToDegrees( reader.ReadUInt8() );
		}
	}
	
	public sealed class EntityLookAndRelativeMoveInbound : PacketS2C {
		int entityId;
		float dx, dy, dz;
		float yaw, pitch;
		
		public override void ReadData( StreamInjector reader ) {
			entityId = reader.ReadInt32();
			dx = reader.ReadInt8() / 32f;
			dy = reader.ReadInt8() / 32f;
			dz = reader.ReadInt8() / 32f;
			yaw = (float)Utils.PackedToDegrees( reader.ReadUInt8() );
			pitch = (float)Utils.PackedToDegrees( reader.ReadUInt8() );
		}
	}
	
	public sealed class EntityTeleportInbound : PacketS2C {
		int entityId;
		float x, y, z;
		float yaw, pitch;
		
		public override void ReadData( StreamInjector reader ) {
			entityId = reader.ReadInt32();
			x = reader.ReadInt32() / 32f;
			y = reader.ReadInt32() / 32f;
			z = reader.ReadInt32() / 32f;
			yaw = (float)Utils.PackedToDegrees( reader.ReadUInt8() );
			pitch = (float)Utils.PackedToDegrees( reader.ReadUInt8() );
		}
	}
	
	public sealed class EntityStatusInbound : PacketS2C {
		int entityId;
		byte status;
		
		public override void ReadData( StreamInjector reader ) {
			entityId = reader.ReadInt32();
			status = reader.ReadUInt8();
		}
	}
	
	public sealed class AttachEntityInbound : PacketS2C {
		int collectedId;
		int collectorId;
		
		public override void ReadData( StreamInjector reader ) {
			collectedId = reader.ReadInt32();
			collectorId = reader.ReadInt32();
		}
	}
	
	public sealed class EntityMetadataInbound : PacketS2C {
		int entityId;
		EntityMetadata meta;
		
		public override void ReadData( StreamInjector reader ) {
			entityId = reader.ReadInt32();
			meta = EntityMetadata.ReadFrom( reader );
		}
	}
	
	public sealed class PrepareChunkInbound : PacketS2C {
		int chunkX, chunkZ;
		bool load;
		
		public override void ReadData( StreamInjector reader ) {
			chunkX = reader.ReadInt32();
			chunkZ = reader.ReadInt32();
			load = reader.ReadBool();
		}
	}
	
	public sealed class MapChunkInbound : PacketS2C {
		int x, y, z;
		int sizeX, sizeY, sizeZ;
		byte[] compressedData;
		
		public override void ReadData( StreamInjector reader ) {
			x = reader.ReadInt32();
			y = reader.ReadInt16();
			z = reader.ReadInt32();
			sizeX = reader.ReadUInt8() + 1;
			sizeY = reader.ReadUInt8() + 1;
			sizeZ = reader.ReadUInt8() + 1;
			int compressedSize = reader.ReadInt32();
			compressedData = reader.ReadRawBytes( compressedSize );
		}
	}
	
	public sealed class MultiBlockChangeInbound : PacketS2C {
		int chunkX, chunkZ;
		ushort[] posMasks;
		byte[] blockTypes;
		byte[] blockMetas;
		
		public override void ReadData( StreamInjector reader ) {
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
	}
	
	public sealed class BlockChangeInbound : PacketS2C {
		int x, y, z;
		byte blockType, blockMeta;
		
		public override void ReadData( StreamInjector reader ) {
			x = reader.ReadInt32();
			y = reader.ReadUInt8();
			z = reader.ReadInt32();
			blockType = reader.ReadUInt8();
			blockMeta = reader.ReadUInt8();
		}
	}
	
	public sealed class BlockActionInbound : PacketS2C {
		int x, y, z;
		byte data1, data2;
		
		public override void ReadData( StreamInjector reader ) {
			x = reader.ReadInt32();
			y = reader.ReadInt16();
			z = reader.ReadInt32();
			data1 = reader.ReadUInt8();
			data2 = reader.ReadUInt8();
		}
	}
	
	public sealed class ExplosionInbound : PacketS2C {
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
		
		public override void ReadData( StreamInjector reader ) {
			x = (float)reader.ReadFloat64();
			y = (float)reader.ReadFloat64();
			z = (float)reader.ReadFloat64();
			radius = reader.ReadFloat32();
			records = new Record[reader.ReadInt32()];
			for( int i = 0; i < records.Length; i++ ) {
				records[i] = new Record( reader.ReadInt8(), reader.ReadInt8(), reader.ReadInt8() );
			}
		}
	}
	
	public sealed class SoundEffectInbound : PacketS2C {
		int effectId;
		int x, y, z;
		int data;
		
		public override void ReadData( StreamInjector reader ) {
			effectId = reader.ReadInt32();
			x = reader.ReadInt32();
			y = reader.ReadInt8();
			z = reader.ReadInt32();
			data = reader.ReadInt32();
		}
	}
	
	public sealed class ChangeGameStateInbound : PacketS2C {
		byte state;
		
		public override void ReadData( StreamInjector reader ) {
			state = reader.ReadUInt8();
		}
	}
	
	public sealed class ThunderboltInbound : PacketS2C {
		int entityId;
		byte unknown;
		float x, y, z;
		
		public override void ReadData( StreamInjector reader ) {
			entityId = reader.ReadInt32();
			unknown = reader.ReadUInt8();
			x = reader.ReadInt32() / 32f;
			y = reader.ReadInt32() / 32f;
			z = reader.ReadInt32() / 32f;
		}
	}
	
	public sealed class OpenWindowInbound : PacketS2C {
		byte windowId;
		string inventoryType;
		string windowTitle;
		byte numSlots;
		
		public override void ReadData( StreamInjector reader ) {
			windowId = reader.ReadUInt8();
			inventoryType = reader.ReadString();
			windowTitle = reader.ReadString();
			numSlots = reader.ReadUInt8();
		}
	}
	
	public sealed class CloseWindowInbound : PacketS2C {
		byte windowId;
		
		public override void ReadData( StreamInjector reader ) {
			windowId = reader.ReadUInt8();
		}
	}
	
	public sealed class SetSlotInbound : PacketS2C {
		byte windowId;
		short slotIndex;
		Slot slot;
		
		public override void ReadData( StreamInjector reader ) {
			windowId = reader.ReadUInt8();
			slotIndex = reader.ReadInt16();
			slot = reader.ReadSlot();
		}
	}
	
	public sealed class WindowItemsInbound : PacketS2C {
		byte windowId;
		Slot[] slots;
		
		public override void ReadData( StreamInjector reader ) {
			windowId = reader.ReadUInt8();
			slots = new Slot[reader.ReadInt16()];
			for( int i = 0; i < slots.Length; i++ ) {
				slots[i] = reader.ReadSlot();
			}
		}
	}
	
	public sealed class WindowPropertyInbound : PacketS2C {
		byte windowId;
		short property;
		short value;
		
		public override void ReadData( StreamInjector reader ) {
			windowId = reader.ReadUInt8();
			property = reader.ReadInt16();
			value = reader.ReadInt16();
		}
	}
	
	public sealed class ConfirmTransactionInbound : PacketS2C {
		byte windowId;
		short actionNum;
		bool accepted;
		
		public override void ReadData( StreamInjector reader ) {
			windowId = reader.ReadUInt8();
			actionNum = reader.ReadInt16();
			accepted = reader.ReadBool();
		}
	}
	
	public sealed class UpdateSignInbound : PacketS2C {
		int x, y, z;
		string line1, line2, line3, line4;
		
		public override void ReadData( StreamInjector reader ) {
			x = reader.ReadInt32();
			y = reader.ReadInt16();
			z = reader.ReadInt32();
			line1 = reader.ReadString();
			line2 = reader.ReadString();
			line3 = reader.ReadString();
			line4 = reader.ReadString();
		}
	}
	
	public sealed class MapsInbound : PacketS2C {
		short itemType;
		short itemDamage;
		byte[] data;
		
		public override void ReadData( StreamInjector reader ) {
			itemType = reader.ReadInt16();
			itemDamage = reader.ReadInt16();
			data = reader.ReadRawBytes( reader.ReadUInt8() );
		}
	}
	
	public sealed class StatisticsInbound : PacketS2C {
		int statId;
		byte amount;
		
		public override void ReadData( StreamInjector reader ) {
			statId = reader.ReadInt32();
			amount = reader.ReadUInt8();
		}
	}
	
	public sealed class DisconnectInbound : PacketS2C {
		string reason;
		
		public override void ReadData( StreamInjector reader ) {
			reason = reader.ReadString();
		}
	}
}
