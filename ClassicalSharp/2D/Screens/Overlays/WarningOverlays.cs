// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using System.Diagnostics;
using ClassicalSharp.Gui.Widgets;
using ClassicalSharp.Network;
using ClassicalSharp.Textures;
using OpenTK.Input;

namespace ClassicalSharp.Gui.Screens {
	
	public sealed class UrlWarningOverlay : Overlay {
		public string Url;
		
		public UrlWarningOverlay(Game game, string url) : base(game) {
			widgets = new Widget[6];
			Url = url;
			lines[0] = "&eAre you sure you want to open this link?";
			lines[1] = url;
			lines[2] = "Be careful - links from strangers may be websites that";
			lines[3] = " have viruses, or things you may not want to open/see.";
		}
		
		protected override void ContextRecreated() {
			MakeLabels();
			widgets[4] = ButtonWidget.Create(game, 160, "Yes", titleFont, OpenUrl)
				.SetLocation(Anchor.Centre, Anchor.Centre, -110, 30);
			widgets[5] = ButtonWidget.Create(game, 160, "No", titleFont, AppendUrl)
				.SetLocation(Anchor.Centre, Anchor.Centre, 110, 30);
		}
		
		void OpenUrl(Game g, Widget w) {
			CloseOverlay();
			try {
				Process.Start(Url);
			} catch (Exception ex) {
				ErrorHandler.LogError("UrlWarningOverlay.OpenUrl", ex);
			}
		}
		
		void AppendUrl(Game g, Widget w) {
			CloseOverlay();
			if (game.ClickableChat) {
				game.Gui.hudScreen.AppendInput(Url);
			}
		}
	}
	
	public abstract class WarningOverlay : Overlay {
		public WarningOverlay(Game game) : base(game) { }
		
		protected override void ContextRecreated() {
			MakeLabels();
			widgets[4] = ButtonWidget.Create(game, 160, "Yes", titleFont, OnYesClick)
				.SetLocation(Anchor.Centre, Anchor.Centre, -110, 30);
			widgets[5] = ButtonWidget.Create(game, 160, "No", titleFont, OnNoClick)
				.SetLocation(Anchor.Centre, Anchor.Centre, 110, 30);
			widgets[6] = ButtonWidget.Create(game, 160, "Always yes", titleFont, OnYesClick)
				.SetLocation(Anchor.Centre, Anchor.Centre, -110, 85);
			widgets[7] = ButtonWidget.Create(game, 160, "Always no", titleFont, OnNoClick)
				.SetLocation(Anchor.Centre, Anchor.Centre, 110, 85);
		}
		
		protected bool IsAlways(Widget w) { return IndexWidget(w) >= 6; }
		
		protected abstract void OnYesClick(Game g, Widget w);
		protected abstract void OnNoClick(Game g, Widget w);
	}
	
	public sealed class PluginOverlay : WarningOverlay {
		public string Plugin;

		public PluginOverlay(Game game, string plugin) : base(game) {
			Plugin = plugin;
			widgets = new Widget[8];
			lines[0] = "&eAre you sure you want to load plugin " + plugin + " ?";
			lines[1] = "Be careful - plugins from strangers may have viruses";
			lines[2] = " or other malicious behaviour.";
		}
		
		protected override void OnYesClick(Game g, Widget w) {
			CloseOverlay();
			EntryList accepted = PluginLoader.Accepted;
			if (IsAlways(w) && !accepted.Has(Plugin)) accepted.Add(Plugin);
			PluginLoader.Load(Plugin, true);
		}
		
		protected override void OnNoClick(Game g, Widget w) {
			CloseOverlay();
			EntryList denied = PluginLoader.Denied;
			if (IsAlways(w) && !denied.Has(Plugin)) denied.Add(Plugin);
		}
	}
	
	public sealed class ConfirmDenyOverlay : Overlay {
		public string Identifier;
		bool alwaysDeny;
		
		public ConfirmDenyOverlay(Game game, bool always) : base(game) {
			alwaysDeny = always;
			widgets = new Widget[6];
			lines[0] = "&eYou might be missing out.";
			lines[1] = "Texture packs can play a vital role in the look and feel of maps.";
			lines[2] = "";
			lines[3] = "Sure you don't want to download the texture pack?";
		}

		protected override void ContextRecreated() {
			MakeLabels();
			widgets[4] = ButtonWidget.Create(game, 160, "I'm sure", titleFont, ConfirmNoClick)
				.SetLocation(Anchor.Centre, Anchor.Centre, -110, 30);
			widgets[5] = ButtonWidget.Create(game, 160, "Go back", titleFont, GoBackClick)
				.SetLocation(Anchor.Centre, Anchor.Centre, 110, 30);
		}
		
		void ConfirmNoClick(Game g, Widget w) {
			CloseOverlay();
			string url = Identifier.Substring(3);
			if (alwaysDeny && !TextureCache.HasDenied(url)) {
				TextureCache.Deny(url);
			}
		}
		
		void GoBackClick(Game g, Widget w) {
			CloseOverlay();
			Overlay overlay = new TexPackOverlay(game, Identifier.Substring(3));
			game.Gui.ShowOverlay(overlay, true);
		}
	}
	
	public sealed class TexPackOverlay : WarningOverlay {
		public string Identifier;

		public TexPackOverlay(Game game, string url) : base(game) {
			string address = url;
			if (url.StartsWith("https://")) address = url.Substring(8);
			if (url.StartsWith("http://"))  address = url.Substring(7);
			Identifier = "CL_" + url;
			game.Downloader.AsyncGetContentLength(url, true, Identifier);
			
			widgets = new Widget[8];
			lines[0] = "Do you want to download the server's texture pack?";
			lines[1] = "Texture pack url:";
			lines[2] = address;
			lines[3] = "Download size: Determining...";
		}
		
		protected override void OnYesClick(Game g, Widget w) {
			CloseOverlay();
			string url = Identifier.Substring(3);
			
			game.Server.DownloadTexturePack(url);
			if (IsAlways(w) && !TextureCache.HasAccepted(url)) {
				TextureCache.Accept(url);
			}
		}
		
		protected override void OnNoClick(Game g, Widget w) {
			CloseOverlay();
			string url = Identifier.Substring(3);
			
			ConfirmDenyOverlay overlay = new ConfirmDenyOverlay(game, IsAlways(w));
			overlay.Identifier = Identifier;
			game.Gui.ShowOverlay(overlay, true);
		}
		
		public override void Render(double delta) {
			base.Render(delta);
			Request item;
			if (!game.Downloader.TryGetItem(Identifier, out item) || item.Data == null) return;
			
			long contentLength = (long)item.Data;
			if (contentLength <= 0) return;
			string url = Identifier.Substring(3);
			
			float contentLengthMB = (contentLength / 1024f / 1024f);
			lines[3] = "Download size: " + contentLengthMB.ToString("F3") + " MB";
			ContextLost();
			ContextRecreated();
		}
	}
}