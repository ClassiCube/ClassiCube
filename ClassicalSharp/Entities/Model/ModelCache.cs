// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using System.Collections.Generic;
using ClassicalSharp.Events;
using ClassicalSharp.GraphicsAPI;

namespace ClassicalSharp.Model {

	public class ModelCache : IDisposable {
		
		Game game;
		IGraphicsApi gfx;
		public ModelCache(Game window) {
			this.game = window;
			gfx = game.Graphics;
		}
		
		#if FALSE
		public CustomModel[] CustomModels = new CustomModel[256];
		#endif
		public List<CachedModel> Models = new List<CachedModel>();
		public List<CachedTexture> Textures = new List<CachedTexture>();
		public int vb;
		public VertexP3fT2fC4b[] vertices;

		public void InitCache() {
			vertices = new VertexP3fT2fC4b[24 * 12];
			RegisterDefaultModels();
			ContextRecreated();
			
			game.Events.TextureChanged += TextureChanged;
			game.Graphics.ContextLost += ContextLost;
			game.Graphics.ContextRecreated += ContextRecreated;
		}
		
		public void Register(string modelName, string texName, IModel instance) {
			CachedModel model = new CachedModel();
			model.Name = modelName;
			model.Instance = instance;
			Models.Add(model);
			instance.texIndex = GetTextureIndex(texName);
		}
		
		public void RegisterTextures(params string[] texNames) {
			for (int i = 0; i < texNames.Length; i++) {
				CachedTexture texture = new CachedTexture();
				texture.Name = texNames[i];
				Textures.Add(texture);
			}
		}
		
		public int GetTextureIndex(string texName) {
			for (int i = 0; i < Textures.Count; i++) {
				if (Textures[i].Name == texName) return i;
			}
			return -1;
		}

		
		public IModel Get(string modelName) {
			for (int i = 0; i < Models.Count; i++) {
				CachedModel m = Models[i];
				if (m.Name != modelName) continue;
				if (!m.Instance.initalised) InitModel(m);
				return m.Instance;
			}
			return Models[0].Instance;
		}
		
		public void Dispose() {			
			for (int i = 0; i < Textures.Count; i++) {
				CachedTexture tex = Textures[i];
				gfx.DeleteTexture(ref tex.TexID);
				Textures[i] = tex;
			}			
			ContextLost();
			
			game.Events.TextureChanged -= TextureChanged;
			game.Graphics.ContextLost -= ContextLost;
			game.Graphics.ContextRecreated -= ContextRecreated;
		}
		
		void InitModel(CachedModel m) {
			m.Instance.CreateParts();
			m.Instance.initalised = true;
		}
		
		void RegisterDefaultModels() {
			RegisterTextures("char.png", "chicken.png", "creeper.png", "pig.png", "sheep.png",
			                 "sheep_fur.png", "skeleton.png", "spider.png", "zombie.png");
			
			Register("humanoid", "char.png", new HumanoidModel(game));
			InitModel(Models[0]);
			SheepModel sheep = new SheepModel(game);
			
			Register("chicken", "chicken.png", new ChickenModel(game));
			Register("creeper", "creeper.png", new CreeperModel(game));
			Register("pig", "pig.png", new PigModel(game));
			Register("sheep", "sheep.png", sheep);
			Register("sheep_nofur", "sheep.png", sheep);
			Register("skeleton", "skeleton.png", new SkeletonModel(game));
			Register("spider", "spider.png", new SpiderModel(game));
			Register("zombie", "zombie.png", new ZombieModel(game));
			
			Register("block", null, new BlockModel(game));
			Register("chibi", "char.png", new ChibiModel(game));
			Register("head", "char.png", new HeadModel(game));
			Register("sit", "char.png", new SittingModel(game));
			Register("sitting", "char.png", new SittingModel(game));
			Register("arm", "char.png", new ArmModel(game));
			Register("corpse", "char.png", new CorpseModel(game));
		}

		void TextureChanged(object sender, TextureEventArgs e) {
			for (int i = 0; i < Textures.Count; i++) {
				CachedTexture tex = Textures[i];
				if (tex.Name != e.Name) continue;
				
				game.UpdateTexture(ref tex.TexID, e.Name, e.Data, e.Name == "char.png");
				Textures[i] = tex; break;
			}
		}
		
		void ContextLost() { game.Graphics.DeleteVb(ref vb); }
		
		void ContextRecreated() {
			vb = gfx.CreateDynamicVb(VertexFormat.P3fT2fC4b, vertices.Length);
		}
	}
	
	public struct CachedModel {
		public string Name;
		public IModel Instance;
	}
	
	public struct CachedTexture {
		public string Name;
		public int TexID;
	}
}
