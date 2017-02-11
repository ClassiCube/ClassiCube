// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using OpenTK;
using ClassicalSharp.Events;
using ClassicalSharp.GraphicsAPI;
using ClassicalSharp.Physics;

namespace ClassicalSharp.Entities {

	public enum NameMode { NoNames, HoveredOnly, All, AllAndHovered, }
	
	public enum EntityShadow { None, SnapToBlock, Circle, CircleAll, }
	
	public sealed class EntityList : IDisposable {
		
		public const int MaxCount = 256;
		public const byte SelfID = 255;
		
		public Entity[] Entities = new Entity[MaxCount];
		public Game game;
		public EntityShadow ShadowMode = EntityShadow.None;
		byte closestId;
		
		/// <summary> Mode of how names of hovered entities are rendered (with or without depth testing),
		/// and how other entity names are rendered. </summary>
		public NameMode NamesMode = NameMode.AllAndHovered;
		
		public EntityList(Game game) {
			this.game = game;
			game.Graphics.ContextLost += ContextLost;
			game.Graphics.ContextRecreated += ContextRecreated;
			game.Events.ChatFontChanged += ChatFontChanged;
			game.Events.TextureChanged += TextureChanged;
			
			NamesMode = Options.GetEnum(OptionsKey.NamesMode, NameMode.HoveredOnly);
			if (game.ClassicMode) NamesMode = NameMode.HoveredOnly;
			ShadowMode = Options.GetEnum(OptionsKey.EntityShadow, EntityShadow.None);
			if (game.ClassicMode) ShadowMode = EntityShadow.None;
		}
		
		/// <summary> Performs a tick call for all player entities contained in this list. </summary>
		public void Tick(ScheduledTask task) {			
			for (int i = 0; i < Entities.Length; i++) {
				if (Entities[i] == null) continue;
				Entities[i].Tick(task.Interval);
			}
		}
		
		/// <summary> Renders the models of all player entities contained in this list. </summary>
		public void RenderModels(IGraphicsApi gfx, double delta, float t) {
			gfx.Texturing = true;
			gfx.AlphaTest = true;
			for (int i = 0; i < Entities.Length; i++) {
				if (Entities[i] == null) continue;
				Entities[i].RenderModel(delta, t);
			}
			gfx.Texturing = false;
			gfx.AlphaTest = false;
		}
		
		/// <summary> Renders the names of all player entities contained in this list.<br/>
		/// If ShowHoveredNames is false, this method only renders names of entities that are
		/// not currently being looked at by the user. </summary>
		public void RenderNames(IGraphicsApi gfx, double delta) {
			if (NamesMode == NameMode.NoNames) return;
			gfx.Texturing = true;
			gfx.AlphaTest = true;
			LocalPlayer localP = game.LocalPlayer;
			Vector3 eyePos = localP.EyePosition;
			closestId = SelfID;
			
			if (NamesMode != NameMode.All)
				closestId = GetClosetPlayer(game.LocalPlayer);
			if (NamesMode == NameMode.HoveredOnly || !game.LocalPlayer.Hacks.CanSeeAllNames) {
				gfx.Texturing = false;
				gfx.AlphaTest = false;
				return;
			}
			
			for (int i = 0; i < Entities.Length; i++) {
				if (Entities[i] == null) continue;
				if (i != closestId || i == SelfID)
					Entities[i].RenderName();
			}
			gfx.Texturing = false;
			gfx.AlphaTest = false;
		}
		
		public void RenderHoveredNames(IGraphicsApi gfx, double delta) {
			if (NamesMode == NameMode.NoNames || NamesMode == NameMode.All)
				return;
			gfx.Texturing = true;
			gfx.AlphaTest = true;
			gfx.DepthTest = false;
			
			for (int i = 0; i < Entities.Length; i++) {
				if (Entities[i] != null && i == closestId && i != SelfID)
					Entities[i].RenderName();
			}
			gfx.Texturing = false;
			gfx.AlphaTest = false;
			gfx.DepthTest = true;
		}
		
		void ContextLost() {
			for (int i = 0; i < Entities.Length; i++) {
				if (Entities[i] == null) continue;
				Entities[i].ContextLost();
			}
		}
		
		void ContextRecreated() {
			for (int i = 0; i < Entities.Length; i++) {
				if (Entities[i] == null) continue;
				Entities[i].ContextRecreated();
			}
		}
		
		void TextureChanged(object sender, TextureEventArgs e) {
			if (e.Name != "char.png") return;
			for (int i = 0; i < Entities.Length; i++) {
				if (Entities[i] == null || Entities[i].TextureId != -1) continue;
				Entities[i].SkinType = game.DefaultPlayerSkinType;				
			}
		}
		
		void ChatFontChanged(object sender, EventArgs e) {
			for (int i = 0; i < Entities.Length; i++) {
				if (Entities[i] == null) continue;
				Player p = Entities[i] as Player;
				if (p != null) p.UpdateName();
			}
		}
		
		/// <summary> Disposes of all player entities contained in this list. </summary>
		public void Dispose() {
			for (int i = 0; i < Entities.Length; i++) {
				if (Entities[i] == null) continue;
				Entities[i].Despawn();
			}
			
			game.Graphics.ContextLost -= ContextLost;
			game.Graphics.ContextRecreated -= ContextRecreated;
			game.Events.ChatFontChanged -= ChatFontChanged;
			game.Events.TextureChanged -= TextureChanged;
			
			if (ShadowComponent.shadowTex > 0)
				game.Graphics.DeleteTexture(ref ShadowComponent.shadowTex);
		}
		
		public byte GetClosetPlayer(Player src) {
			Vector3 eyePos = src.EyePosition;
			Vector3 dir = Utils.GetDirVector(src.HeadYRadians, src.HeadXRadians);
			float closestDist = float.PositiveInfinity;
			byte targetId = SelfID;
			
			for (int i = 0; i < Entities.Length - 1; i++) { // -1 because we don't want to pick against local player
				Entity p = Entities[i];
				if (p == null) continue;
				
				float t0, t1;
				if (Intersection.RayIntersectsRotatedBox(eyePos, dir, p, out t0, out t1) && t0 < closestDist) {
					closestDist = t0;
					targetId = (byte)i;
				}
			}
			return targetId;
		}
		
		/// <summary> Gets or sets the player entity for the specified id. </summary>
		public Entity this[int id] {
			get { return Entities[id]; }
			set {
				Entities[id] = value;
				if (value != null)
					value.ID = (byte)id;
			}
		}
		
		public void DrawShadows() {
			if (ShadowMode == EntityShadow.None) return;
			ShadowComponent.boundShadowTex = false;
			IGraphicsApi gfx = game.Graphics;
			
			gfx.AlphaArgBlend = true;
			gfx.DepthWrite = false;
			gfx.AlphaBlending = true;
			gfx.Texturing = true;
			
			gfx.SetBatchFormat(VertexFormat.P3fT2fC4b);
			((Player)Entities[SelfID]).shadow.Draw();
			if (ShadowMode == EntityShadow.CircleAll)
				DrawOtherShadows();
			
			gfx.AlphaArgBlend = false;
			gfx.DepthWrite = true;
			gfx.AlphaBlending = false;
			gfx.Texturing = false;
		}
		
		void DrawOtherShadows() {
			for (int i = 0; i < SelfID; i++) {
				if (Entities[i] == null) continue;
				Player p = Entities[i] as Player;
				if (p != null) p.shadow.Draw();
			}
		}
	}
}
