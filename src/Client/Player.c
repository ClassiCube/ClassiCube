#include "Player.h"
#include "Platform.h"
#include "GraphicsAPI.h"
#include "Drawer2D.h"
#include "Particle.h"
#include "GraphicsCommon.h"
#include "AsyncDownloader.h"
#include "ExtMath.h"

#define PLAYER_NAME_EMPTY_TEX -30000
void Player_MakeNameTexture(Player* player) {
	FontDesc font; 
	Platform_MakeFont(&font, &Game_FontName, 24, FONT_STYLE_NORMAL);

	String displayName = String_FromRawArray(player->DisplayNameRaw);
	DrawTextArgs args; 
	DrawTextArgs_Make(&args, &displayName, &font, false);

	/* we want names to always be drawn not using the system font */
	bool bitmapped = Drawer2D_UseBitmappedChat;
	Drawer2D_UseBitmappedChat = true;
	Size2D size = Drawer2D_MeasureText(&args);

	if (size.Width == 0) {
		player->NameTex = Texture_MakeInvalid();
		player->NameTex.X = PLAYER_NAME_EMPTY_TEX;
	} else {
		UInt8 buffer[String_BufferSize(STRING_SIZE * 2)];
		String shadowName = String_InitAndClearArray(buffer);

		size.Width += 3; size.Height += 3;
		Bitmap bmp; Bitmap_AllocatePow2(&bmp, size.Width, size.Height);
		Drawer2D_Begin(&bmp);

		Drawer2D_Cols[0xFF] = PackedCol_Create3(80, 80, 80);
		String_Append(&shadowName, 0xFF);
		String_AppendColorless(&shadowName, &displayName);
		args.Text = shadowName;
		Drawer2D_DrawText(&args, 3, 3);

		Drawer2D_Cols[0xFF] = PackedCol_Create4(0, 0, 0, 0);
		args.Text = displayName;
		Drawer2D_DrawText(&args, 0, 0);

		Drawer2D_End();
		player->NameTex = Drawer2D_Make2DTexture(&bmp, size, 0, 0);
	}

	Drawer2D_UseBitmappedChat = bitmapped;
}

void Player_UpdateName(Player* player) {
	Entity* entity = &player->Base;
	entity->VTABLE->ContextLost(entity);

	if (Gfx_LostContext) return;
	Player_MakeNameTexture(player);
}

void Player_DrawName(Player* player) {
	Entity* entity = &player->Base;
	IModel* model = entity->Model;

	if (player->NameTex.X == PLAYER_NAME_EMPTY_TEX) return;
	if (player->NameTex.ID == NULL) Player_MakeNameTexture(player);
	Gfx_BindTexture(player->NameTex.ID);

	Vector3 pos;
	model->RecalcProperties(entity);
	Vector3_TransformY(&pos, model->NameYOffset, &entity->Transform);
	Real32 scale = Math.Min(1.0f, model->NameScale * entity->ModelScale.Y) / 70.0f;
	PackedCol col = PACKEDCOL_WHITE;
	Vector2 size = Vector2_Create2(player->NameTex.Width * scale, player->NameTex.Height * scale);

	if (Entities_NameMode == NAME_MODE_ALL_UNSCALED && LocalPlayer_Instance.Hacks.CanSeeAllNames) {
		/* Get W component of transformed position */
		Matrix mat;
		Matrix_Mul(&mat, &Gfx_View, &Gfx_Projection); /* TODO: This mul is slow, avoid it */
		Real32 tempW = pos.X * mat.Row0.W + pos.Y * mat.Row1.W + pos.Z * mat.Row2.W + mat.Row3.W;
		size.X *= tempW * 0.2f; size.Y *= tempW * 0.2f;
	}

	VertexP3fT2fC4b vertices[4]; 
	VertexP3fT2fC4b* ptr = vertices;
	TextureRec rec; rec.U1 = 0.0f; rec.V1 = 0.0f; rec.U2 = player->NameTex.U2; rec.V2 = player->NameTex.V2;
	Particle_DoRender(&size, &pos, &rec, col, &ptr);

	Gfx_SetBatchFormat(VERTEX_FORMAT_P3FT2FC4B);
	GfxCommon_UpdateDynamicVb_IndexedTris(GfxCommon_texVb, ptr, 4);
}

Player* Player_FirstOtherWithSameSkin(Player* player) {
	Entity* entity = &player->Base;
	String skin = String_FromRawArray(player->SkinNameRaw);
	UInt32 i;
	for (i = 0; i < ENTITIES_MAX_COUNT; i++) {
		if (Entities_List[i] == NULL || Entities_List[i] == entity) continue;
		if (Entities_List[i]->EntityType != ENTITY_TYPE_PLAYER) continue;

		Player* p = (Player*)Entities_List[i];
		String pSkin = String_FromRawArray(p->SkinNameRaw);
		if (String_Equals(&skin, &pSkin)) return p;
	}
	return NULL;
}

Player* Player_FirstOtherWithSameSkinAndFetchedSkin(Player* player) {
	Entity* entity = &player->Base;
	String skin = String_FromRawArray(player->SkinNameRaw);
	UInt32 i;
	for (i = 0; i < ENTITIES_MAX_COUNT; i++) {
		if (Entities_List[i] == NULL || Entities_List[i] == entity) continue;
		if (Entities_List[i]->EntityType != ENTITY_TYPE_PLAYER) continue;

		Player* p = (Player*)Entities_List[i];
		String pSkin = String_FromRawArray(p->SkinNameRaw);
		if (p->FetchedSkin && String_Equals(&skin, &pSkin)) return p;
	}
	return NULL;
}

void Player_ApplySkin(Player* player, Player* from) {
	Entity* dst = &player->Base;
	Entity* src = &from->Base;

	dst->TextureId    = src->TextureId;	
	dst->SkinType     = src->SkinType;
	dst->uScale       = src->uScale;
	dst->vScale       = src->vScale;

	/* Custom mob textures */
	dst->MobTextureId = NULL;
	String skin = String_FromRawArray(player->SkinNameRaw);
	if (Utils_IsUrlPrefix(&skin, 0)) {
		dst->MobTextureId = dst->TextureId;
	}
}

void Player_ResetSkin(Player* player) {
	Entity* entity = &player->Base;
	entity->uScale = 1; entity->vScale = 1.0f;
	entity->MobTextureId = NULL;
	entity->TextureId    = NULL;
	entity->SkinType = Game_DefaultPlayerSkinType;
}

/* Apply or reset skin, for all players with same skin */
void Player_SetSkinAll(Player* player, bool reset) {
	Entity* entity = &player->Base;
	String skin = String_FromRawArray(player->SkinNameRaw);
	UInt32 i;
	for (i = 0; i < ENTITIES_MAX_COUNT; i++) {
		if (Entities_List[i] == NULL || Entities_List[i] == entity) continue;
		if (Entities_List[i]->EntityType != ENTITY_TYPE_PLAYER) continue;

		Player* p = (Player*)Entities_List[i];
		String pSkin = String_FromRawArray(p->SkinNameRaw);
		if (!String_Equals(&skin, &pSkin)) continue;

		if (reset) {
			Player_ResetSkin(p);
		} else {
			Player_ApplySkin(p, player);
		}
	}
}

void Player_ClearHat(Bitmap bmp, UInt8 skinType) {
	Int32 sizeX = (bmp.Width / 64) * 32;
	Int32 yScale = skinType == SKIN_TYPE_64x32 ? 32 : 64;
	Int32 sizeY = (bmp.Height / yScale) * 16;
	Int32 x, y;

	/* determine if we actually need filtering */
	for (y = 0; y < sizeY; y++) {
		UInt32* row = Bitmap_GetRow(&bmp, y);
		row += sizeX;
		for (x = 0; x < sizeX; x++) {
			UInt8 alpha = (UInt8)(row[x] >> 24);
			if (alpha != 255) return;
		}
	}

	/* only perform filtering when the entire hat is opaque */
	UInt32 fullWhite = PackedCol_ARGB(255, 255, 255, 255);
	UInt32 fullBlack = PackedCol_ARGB(0,   0,   0,   255);
	for (y = 0; y < sizeY; y++) {
		UInt32* row = Bitmap_GetRow(&bmp, y);
		row += sizeX;
		for (x = 0; x < sizeX; x++) {
			UInt32 pixel = row[x];
			if (pixel == fullWhite || pixel == fullBlack) row[x] = 0;
		}
	}
}

void Player_EnsurePow2(Player* player, Bitmap* bmp) {
	Int32 width  = Math_NextPowOf2(bmp->Width);
	Int32 height = Math_NextPowOf2(bmp->Height);
	if (width == bmp->Width && height == bmp->Height) return;

	Bitmap scaled; Bitmap_Allocate(&scaled, width, height);	
	Int32 y;
	for (y = 0; y < bmp->Height; y++) {
		Bitmap_CopyRow(y, y, bmp, &scaled, bmp->Width);
	}

	Entity* entity = &player->Base;
	entity->uScale = (Real32)bmp->Width  / width;
	entity->vScale = (Real32)bmp->Height / height;

	Platform_MemFree(bmp->Scan0);
	*bmp = scaled;
}

void Player_CheckSkin(Player* player) {
	Entity* entity = &player->Base;
	String skin = String_FromRawArray(player->SkinNameRaw);

	if (!player->FetchedSkin && entity->Model->UsesSkin) {
		Player* first = Player_FirstOtherWithSameSkinAndFetchedSkin(player);
		if (first == NULL) {
			AsyncDownloader_DownloadSkin(&skin, &skin);
		} else {
			Player_ApplySkin(player, first);
		}
		player->FetchedSkin = true;
	}

	AsyncRequest item;
	if (!AsyncDownloader_Get(&skin, &item)) return;
	if (item.Data == NULL) { Player_SetSkinAll(player, true); return; }

	Bitmap bmp = (Bitmap)item.Data;
	Gfx_DeleteTexture(&entity->TextureId);

	Player_SetSkinAll(player, true);
	Player_EnsurePow2(player, &bmp);
	entity->SkinType = Utils_GetSkinType(&bmp);

	if (entity->SkinType == SKIN_TYPE_INVALID) {
		Player_SetSkinAll(player, true);
	} else {
		if (entity->Model->UsesHumanSkin) {
			Player_ClearHat(bmp, entity->SkinType);
		}
		entity->TextureId = Gfx_CreateTexture(&bmp, true, false);
		Player_SetSkinAll(player, false);
	}
	Platform_MemFree(bmp.Scan0);
}

void Player_Despawn(Entity* entity) {
	Player* player = (Player*)entity;
	Player* first = Player_FirstOtherWithSameSkin(player);
	if (first == NULL) {
		Gfx_DeleteTexture(&entity->TextureId);
		player->NameTex = Texture_MakeInvalid();
		Player_ResetSkin(player);
	}
	entity->VTABLE->ContextLost(entity);
}

void Player_ContextLost(Entity* entity) {
	Player* player = (Player*)entity;
	Gfx_DeleteTexture(&player->NameTex.ID);
	player->NameTex = Texture_MakeInvalid();
}

void Player_ContextRecreated(Entity* entity) {
	Player* player = (Player*)entity;
	Player_UpdateName(player);
}

void Player_Init(Player* player) {
	/* TODO should we just remove the memset from entity_Init and player_init?? */
	Platform_MemSet(player + sizeof(Entity), 0, sizeof(Player) - sizeof(Entity));
	Entity* entity = &player->Base;
	Entity_Init(entity);
	entity->StepSize = 0.5f;
	entity->EntityType = ENTITY_TYPE_PLAYER;
	String model = String_FromConst("humanoid");
	Entity_SetModel(entity, &model);
	/* TODO: Init vtable */
}
