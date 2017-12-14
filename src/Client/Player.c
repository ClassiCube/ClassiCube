#include "Player.h"
#include "Platform.h"
#include "GraphicsAPI.h"
#include "Drawer2D.h"

void Player_Init(Player* player) {
	/* TODO should we just remove the memset from entity_Init and player_init?? */
	Platform_MemSet(player + sizeof(Entity), 0, sizeof(Player) - sizeof(Entity));
	Entity* entity = &player->Base;
	Entity_Init(entity);
	entity->StepSize = 0.5f;
	entity->EntityType = ENTITY_TYPE_PLAYER;
	String model = String_FromConst("humanoid");
	Entity_SetModel(&entity, &model);
	/* TODO: Init vtable */
}

void Player_Despawn(Entity* entity) {
	Player* player = (Player*)entity;
	Player* first = FirstOtherWithSameSkin();
	if (first == NULL) {
		Gfx_DeleteTexture(&entity->TextureId);
		ResetSkin();
	}
	entity->VTABLE->ContextLost(entity);
}

void Player_ContextLost(Entity* entity) {
	Player* player = (Player*)entity;
	Gfx_DeleteTexture(&player->NameTex.ID);
}

void Player_ContextRecreated(Entity* entity) {
	Player* player = (Player*)entity;
	Player_UpdateName(player); 
}

void Player_MakeNameTexture(Player* player) {
	using (Font font = new Font(game.FontName, 24)) {
		String displayName = String_FromRaw(player->DisplayNameRaw, STRING_SIZE);
		DrawTextArgs args = new DrawTextArgs(displayName, font, false);
		bool bitmapped = Drawer2D_UseBitmappedChat; /* we want names to always be drawn without */
		Drawer2D_UseBitmappedChat = true;
		Size2D size = Drawer2D_MeasureSize(&args);

		if (size.Width == 0) {
			nameTex = new Texture(-1, 0, 0, 0, 0, 1, 1);
		} else {
			UInt8 buffer[String_BufferSize(STRING_SIZE * 2)];
			String shadowName = String_InitAndClear(buffer, STRING_SIZE * 2);

			size.Width += 3; size.Height += 3;
			Bitmap bmp; Bitmap_AllocatePow2(&bmp, size.Width, size.Height);
			Drawer2D_Begin(&bmp);

			Drawer2D_Cols[0xFF] = PackedCol_Create3(80, 80, 80);
			String_Append(&shadowName, 0xFF);
			String_AppendColorless(&shadowName, &displayName);
			args->Text = shadowName;
			Drawer2D_DrawText(args, 3, 3);

			Drawer2D_Cols[0xFF] = PackedCol_Create4(0, 0, 0, 0);
			args->Text = displayName;
			Drawer2D_DrawText(args, 0, 0);

			Drawer2D_End();
			nameTex = Drawer2D_Make2DTexture(&bmp, size, 0, 0);
		}

		Drawer2D_UseBitmappedChat = bitmapped;
	}
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
	if (nameTex.ID == 0) MakeNameTexture();
	if (nameTex.ID == -1) return;

	Gfx_BindTexture(player->NameTex.ID);

	Vector3 pos;
	model->RecalcProperties(entity);
	Vector3_TransformY(&pos, model->NameYOffset, &entity->Transform);
	Real32 scale = Math.Min(1, Model.NameScale * ModelScale.Y) / 70.0f;
	Int32 col = FastColour.WhitePacked;
	Vector2 size = Vector2_Create2(nameTex.Width * scale, nameTex.Height * scale);

	if (Entities_NameMode == NAME_MODE_ALL_UNSCALED && game.LocalPlayer.Hacks.CanSeeAllNames) {
		/* Get W component of transformed position */
		Matrix mat;
		Matrix_Mul(&mat, &Game_View, &Game_Projection); /* TODO: This mul is slow, avoid it */
		Real32 tempW = pos.X * mat.Row0.W + pos.Y * mat.Row1.W + pos.Z * mat.Row2.W + mat.Row3.W;
		size.X *= tempW * 0.2f; size.Y *= tempW * 0.2f;
	}

	Int32 index = 0;
	TextureRec rec; rec.U1 = 0.0f; rec.V1 = 0.0f; rec.U2 = nameTex.U2; rec.V2 = nameTex.V2;
	Particle.DoRender(game, ref size, ref pos, ref rec, col, gfx.texVerts, ref index);

	gfx.SetBatchFormat(VertexFormat.P3fT2fC4b);
	gfx.UpdateDynamicVb_IndexedTris(gfx.texVb, gfx.texVerts, 4);
}

void Player_CheckSkin(Player* player) {
	if (!fetchedSkin && Model.UsesSkin) {
		Player first = FirstOtherWithSameSkinAndFetchedSkin();
		if (first == null) {
			game.AsyncDownloader.DownloadSkin(SkinName, SkinName);
		} else {
			ApplySkin(first);
		}
		fetchedSkin = true;
	}

	Request item;
	if (!game.AsyncDownloader.TryGetItem(SkinName, out item)) return;
	if (item == null || item.Data == null) { SetSkinAll(true); return; }

	Bitmap bmp = (Bitmap)item.Data;
	game.Graphics.DeleteTexture(ref TextureId);

	SetSkinAll(true);
	EnsurePow2(ref bmp);
	SkinType = Utils.GetSkinType(bmp);

	if (SkinType == SkinType.Invalid) {
		SetSkinAll(true);
	} else {
		if (Model.UsesHumanSkin) ClearHat(bmp, SkinType);
		TextureId = game.Graphics.CreateTexture(bmp, true, false);
		SetSkinAll(false);
	}
	bmp.Dispose();
}

Player* Player_FirstOtherWithSameSkin(Player* player) {
	UInt32 i;
	for (i = 0; i < ENTITIES_MAX_COUNT; i++) {
		if (Entities_List[i] == NULL || Entities_List[i] == player) continue;
		if (Entities_List[i]->EntityType != ENTITY_TYPE_PLAYER) continue;

		Player* p = (Player*)Entities_List[i];
		if (p.SkinName == SkinName) return p;
	}
	return NULL;
}

Player* Player_FirstOtherWithSameSkinAndFetchedSkin(Player* player) {
	UInt32 i;
	for (i = 0; i < ENTITIES_MAX_COUNT; i++) {
		if (Entities_List[i] == NULL || Entities_List[i] == player) continue;
		if (Entities_List[i]->EntityType != ENTITY_TYPE_PLAYER) continue;

		Player* p = (Player*)Entities_List[i];
		if (p.SkinName == SkinName && p->FetchedSkin) return p;
	}
	return NULL;
}

/* Apply or reset skin, for all players with same skin */
void Player_SetSkinAll(Player* player, bool reset) {
	UInt32 i;
	for (i = 0; i < ENTITIES_MAX_COUNT; i++) {
		if (Entities_List[i] == NULL || Entities_List[i] == player) continue;
		if (Entities_List[i]->EntityType != ENTITY_TYPE_PLAYER) continue;

		Player* p = (Player*)Entities_List[i];
		if (p.SkinName != SkinName) continue;

		if (reset) { p.ResetSkin(); } else { p.ApplySkin(this); }
	}
}

void Player_ApplySkin(Player* player, Player* from) {
	TextureId = from.TextureId;
	MobTextureId = -1;
	SkinType = from.SkinType;
	uScale = from.uScale; vScale = from.vScale;

	/* Custom mob textures */
	if (Utils.IsUrlPrefix(SkinName, 0)) MobTextureId = TextureId;
}

void Player_ResetSkin(Player* player) {
	uScale = 1; vScale = 1;
	MobTextureId = -1;
	TextureId = -1;
	SkinType = game.DefaultPlayerSkinType;
}

void Player_ClearHat(Bitmap bmp, UInt8 skinType) {
	Int32 sizeX = (bmp.Width / 64) * 32;
	Int32 yScale = skinType == SkinType_64x32 ? 32 : 64;
	Int32 sizeY = (bmp.Height / yScale) * 16;

	/* determine if we actually need filtering */
	for (Int32 y = 0; y < sizeY; y++) {
		int* row = fastBmp.GetRowPtr(y);
		row += sizeX;
		for (Int32 x = 0; x < sizeX; x++) {
			byte alpha = (byte)(row[x] >> 24);
			if (alpha != 255) return;
		}
	}

	/* only perform filtering when the entire hat is opaque */
	UInt32 fullWhite = PackedCol_ARGB(255, 255, 255, 255);
	UInt32 fullBlack = PackedCol_ARGB(0,   0,   0,   255);
	for (Int32 y = 0; y < sizeY; y++) {
		int* row = fastBmp.GetRowPtr(y);
		row += sizeX;
		for (Int32 x = 0; x < sizeX; x++) {
			Int32 pixel = row[x];
			if (pixel == fullWhite || pixel == fullBlack) row[x] = 0;
		}
	}
}

void Player_EnsurePow2(ref Bitmap bmp) {
	Int32 width = Utils.NextPowerOf2(bmp.Width);
	Int32 height = Utils.NextPowerOf2(bmp.Height);
	if (width == bmp.Width && height == bmp.Height) return;

	Bitmap scaled = Platform.CreateBmp(width, height);
	using (FastBitmap src = new FastBitmap(bmp, true, true))
		using (FastBitmap dst = new FastBitmap(scaled, true, false))
	{
		for (Int32 y = 0; y < src.Height; y++)
			FastBitmap.CopyRow(y, y, src, dst, src.Width);
	}

	uScale = (float)bmp.Width / width;
	vScale = (float)bmp.Height / height;
	bmp.Dispose();
	bmp = scaled;
}