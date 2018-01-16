// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using OpenTK;
using ClassicalSharp.Events;
using ClassicalSharp.GraphicsAPI;
using ClassicalSharp.Physics;

namespace ClassicalSharp.Entities {

	public enum NameMode { None, Hovered, All, AllHovered, AllUnscaled }
	
	public enum EntityShadow { None, SnapToBlock, Circle, CircleAll, }
	
	public sealed class EntityList : IDisposable {
		
		public const int MaxCount = 256;
		public const byte SelfID = 255;
		
		public Entity[] List = new Entity[MaxCount];
		public Game game;
		public EntityShadow ShadowMode = EntityShadow.None;
		byte closestId;
		
		/// <summary> Mode of how names of hovered entities are rendered (with or without depth testing),
		/// and how other entity names are rendered. </summary>
		public NameMode NamesMode = NameMode.Hovered;
		
		public EntityList(Game game) {
			this.game = game;
			game.Graphics.ContextLost += ContextLost;
			game.Graphics.ContextRecreated += ContextRecreated;
			game.Events.ChatFontChanged += ChatFontChanged;
			
			NamesMode = Options.GetEnum(OptionsKey.NamesMode, NameMode.Hovered);
			if (game.ClassicMode) NamesMode = NameMode.Hovered;
			ShadowMode = Options.GetEnum(OptionsKey.EntityShadow, EntityShadow.None);
			if (game.ClassicMode) ShadowMode = EntityShadow.None;
		}
		
		/// <summary> Performs a tick call for all player entities contained in this list. </summary>
		public void Tick(ScheduledTask task) {
			for (int i = 0; i < List.Length; i++) {
				if (List[i] == null) continue;
				List[i].Tick(task.Interval);
			}
		}
		
		/// <summary> Renders the models of all player entities contained in this list. </summary>
		public void RenderModels(IGraphicsApi gfx, double delta, float t) {
			gfx.Texturing = true;
			gfx.AlphaTest = true;
			for (int i = 0; i < List.Length; i++) {
				if (List[i] == null) continue;
				List[i].RenderModel(delta, t);
			}
			gfx.Texturing = false;
			gfx.AlphaTest = false;
		}
		bool hadFog;
		
		/// <summary> Renders the names of all player entities contained in this list.<br/>
		/// If ShowHoveredNames is false, this method only renders names of entities that are
		/// not currently being looked at by the user. </summary>
		public void RenderNames(IGraphicsApi gfx, double delta) {
			if (NamesMode == NameMode.None) return;
			closestId = GetClosetPlayer(game.LocalPlayer);
			if (!game.LocalPlayer.Hacks.CanSeeAllNames || NamesMode != NameMode.All) return;

			gfx.Texturing = true;
			gfx.AlphaTest = true;
			hadFog = gfx.Fog;
			if (hadFog) gfx.Fog = false;
			
			for (int i = 0; i < List.Length; i++) {
				if (List[i] == null) continue;
				if (i != closestId || i == SelfID) {
					List[i].RenderName();
				}
			}
			
			gfx.Texturing = false;
			gfx.AlphaTest = false;
			if (hadFog) gfx.Fog = true;
		}
		
		public void RenderHoveredNames(IGraphicsApi gfx, double delta) {
			if (NamesMode == NameMode.None) return;
			gfx.Texturing = true;
			gfx.AlphaTest = true;
			gfx.DepthTest = false;
			hadFog = gfx.Fog;
			if (hadFog) gfx.Fog = false;
			
			bool allNames = !(NamesMode == NameMode.Hovered || NamesMode == NameMode.All)
				&& game.LocalPlayer.Hacks.CanSeeAllNames;
			for (int i = 0; i < List.Length; i++) {
				bool hover = (i == closestId || allNames) && i != SelfID;
				if (List[i] != null && hover) {
					List[i].RenderName();
				}
			}
			
			gfx.Texturing = false;
			gfx.AlphaTest = false;
			gfx.DepthTest = true;
			if (hadFog) gfx.Fog = true;
		}
		
		void ContextLost() {
			for (int i = 0; i < List.Length; i++) {
				if (List[i] == null) continue;
				List[i].ContextLost();
			}
			game.Graphics.DeleteTexture(ref ShadowComponent.shadowTex);
		}
		
		void ContextRecreated() {
			for (int i = 0; i < List.Length; i++) {
				if (List[i] == null) continue;
				List[i].ContextRecreated();
			}
		}
		
		void ChatFontChanged(object sender, EventArgs e) {
			for (int i = 0; i < List.Length; i++) {
				if (List[i] == null) continue;
				Player p = List[i] as Player;
				if (p != null) p.UpdateName();
			}
		}
		
		public void RemoveEntity(byte id) {
			game.EntityEvents.RaiseRemoved(id);
			List[id].Despawn();
			List[id] = null;
		}
		
		/// <summary> Disposes of all player entities contained in this list. </summary>
		public void Dispose() {
			for (int i = 0; i < List.Length; i++) {
				if (List[i] == null) continue;
				RemoveEntity((byte)i);
			}
			
			game.Graphics.ContextLost -= ContextLost;
			game.Graphics.ContextRecreated -= ContextRecreated;
			game.Events.ChatFontChanged -= ChatFontChanged;
			
			if (ShadowComponent.shadowTex > 0)
				game.Graphics.DeleteTexture(ref ShadowComponent.shadowTex);
		}
		
		public byte GetClosetPlayer(Player src) {
			Vector3 eyePos = src.EyePosition;
			Vector3 dir = Utils.GetDirVector(src.HeadYRadians, src.HeadXRadians);
			float closestDist = float.PositiveInfinity;
			byte targetId = SelfID;
			
			for (int i = 0; i < List.Length - 1; i++) { // -1 because we don't want to pick against local player
				Entity p = List[i];
				if (p == null) continue;
				
				float t0, t1;
				if (Intersection.RayIntersectsRotatedBox(eyePos, dir, p, out t0, out t1) && t0 < closestDist) {
					closestDist = t0;
					targetId = (byte)i;
				}
			}
			return targetId;
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
			ShadowComponent.Draw(game, List[SelfID]);
			if (ShadowMode == EntityShadow.CircleAll) {
				for (int i = 0; i < SelfID; i++) {
					if (List[i] == null) continue;
					Player p = List[i] as Player;
					if (p != null) ShadowComponent.Draw(game, p);
				}
			}
			
			gfx.AlphaArgBlend = false;
			gfx.DepthWrite = true;
			gfx.AlphaBlending = false;
			gfx.Texturing = false;
		}
	}
}
