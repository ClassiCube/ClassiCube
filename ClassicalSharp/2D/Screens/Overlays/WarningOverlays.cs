// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using System.Diagnostics;
using ClassicalSharp.Gui.Widgets;
using ClassicalSharp.Network;
using ClassicalSharp.Textures;
using OpenTK.Input;

namespace ClassicalSharp.Gui.Screens {
	
	public sealed class UrlWarningOverlay : Overlay {
		
		public UrlWarningOverlay(Game game, string url) : base(game) {
			widgets = new ButtonWidget[2];
			Metadata = url;
			lines[0] = "&eAre you sure you want to open this link?";
			lines[1] = url;
			lines[2] = "Be careful - links from strangers may be websites that";
			lines[3] = " have viruses, or things you may not want to open/see.";
		}
		
		public override void MakeButtons() {
			DisposeWidgets(widgets);
			widgets[0] = ButtonWidget.Create(game, 160, "Yes", titleFont, OpenUrl)
				.SetLocation(Anchor.Centre, Anchor.Centre, -110, 30);
			widgets[1] = ButtonWidget.Create(game, 160, "No", titleFont, AppendUrl)
				.SetLocation(Anchor.Centre, Anchor.Centre, 110, 30);
		}
		
		void OpenUrl(Game g, Widget w) {
			CloseOverlay();			
			try {
				Process.Start(Metadata);
			} catch (Exception ex) {
				ErrorHandler.LogError("UrlWarningOverlay.OpenUrl", ex);
			}
		}
		
		void AppendUrl(Game g, Widget w) {
			CloseOverlay();			
			if (game.ClickableChat) {
				game.Gui.hudScreen.AppendInput(Metadata);
			}
		}
	}
	
	public abstract class WarningOverlay : Overlay {
		public WarningOverlay(Game game) : base(game) { }
		
		public override void MakeButtons() {
			DisposeWidgets(widgets);
			widgets[0] = ButtonWidget.Create(game, 160, "Yes", titleFont, OnYesClick)
				.SetLocation(Anchor.Centre, Anchor.Centre, -110, 30);
			widgets[1] = ButtonWidget.Create(game, 160, "No", titleFont, OnNoClick)
				.SetLocation(Anchor.Centre, Anchor.Centre, 110, 30);
			widgets[2] = ButtonWidget.Create(game, 160, "Always yes", titleFont, OnYesClick)
				.SetLocation(Anchor.Centre, Anchor.Centre, -110, 85);
			widgets[3] = ButtonWidget.Create(game, 160, "Always no", titleFont, OnNoClick)
				.SetLocation(Anchor.Centre, Anchor.Centre, 110, 85);
		}
		
		protected abstract void OnYesClick(Game g, Widget w);
		protected abstract void OnNoClick(Game g, Widget w);
	}
	
	public sealed class PluginOverlay : WarningOverlay {

		public PluginOverlay(Game game, string plugin) : base(game) {
			Metadata = plugin;
			widgets = new ButtonWidget[4];
			lines[0] = "&eAre you sure you want to load plugin " + plugin + " ?";
			lines[1] = "Be careful - plugins from strangers may have viruses";
			lines[2] = " or other malicious behaviour.";
		}
		
		protected override void OnYesClick(Game g, Widget w) {
			CloseOverlay();			
			EntryList accepted = PluginLoader.Accepted;
			if (IndexOfWidget(w) >= 2 && !accepted.Has(Metadata)) accepted.Add(Metadata);
			PluginLoader.Load(Metadata, true);
		}
		
		protected override void OnNoClick(Game g, Widget w) {
			CloseOverlay();			
			EntryList denied = PluginLoader.Denied;
			if (IndexOfWidget(w) >= 2 && !denied.Has(Metadata)) denied.Add(Metadata);
		}
	}
	
	public sealed class ConfirmDenyOverlay : Overlay {
		bool alwaysDeny;
		
		public ConfirmDenyOverlay(Game game, bool always) : base(game) {
			alwaysDeny = always;
			widgets = new ButtonWidget[2];
			lines[0] = "&eYou might be missing out.";
			lines[1] = "Texture packs can play a vital role in the look and feel of maps.";
			lines[2] = "";
			lines[3] = "Sure you don't want to download the texture pack?";
		}

		public override void MakeButtons() {
			DisposeWidgets(widgets);
			widgets[0] = ButtonWidget.Create(game, 160, "I'm sure", titleFont, ConfirmNoClick)
				.SetLocation(Anchor.Centre, Anchor.Centre, -110, 30);
			widgets[1] = ButtonWidget.Create(game, 160, "Go back", titleFont, GoBackClick)
				.SetLocation(Anchor.Centre, Anchor.Centre, 110, 30);
		}
		
		void ConfirmNoClick(Game g, Widget w) {
			CloseOverlay();			
			string url = Metadata.Substring(3);
			if (alwaysDeny && !game.DeniedUrls.Has(url)) {
				game.DeniedUrls.Add(url);
			}
		}
		
		void GoBackClick(Game g, Widget w) {
			CloseOverlay();			
			Overlay overlay = new TexPackOverlay(game, Metadata.Substring(3));
			game.Gui.ShowOverlay(overlay, true);
		}
	}
	
	public sealed class TexPackOverlay : WarningOverlay {

		public TexPackOverlay(Game game, string url) : base(game) {
			string address = url;
			if (url.StartsWith("https://")) address = url.Substring(8);
			if (url.StartsWith("http://"))  address = url.Substring(7);
			Metadata = "CL_" + url;
			OnRenderFrame = TexPackTick;
			
			widgets = new ButtonWidget[4];
			lines[0] = "Do you want to download the server's texture pack?";
			lines[1] = "Texture pack url:";
			lines[2] = address;
			lines[3] = "Download size: Determining...";
		}
		
		protected override void OnYesClick(Game g, Widget w) {
			CloseOverlay();			
			string url = Metadata.Substring(3);
			
			game.Server.DownloadTexturePack(url);
			if (IndexOfWidget(w) >= 2 && !game.AcceptedUrls.Has(url)) {
				game.AcceptedUrls.Add(url);
			}
		}
		
		protected override void OnNoClick(Game g, Widget w) {
			CloseOverlay();
			string url = Metadata.Substring(3);
			
			ConfirmDenyOverlay overlay = new ConfirmDenyOverlay(game, IndexOfWidget(w) >= 2);
			overlay.Metadata = Metadata;
			game.Gui.ShowOverlay(overlay, true);
		}
		
		void TexPackTick(Overlay warning) {
			string identifier = warning.Metadata;
			Request item;
			if (!game.Downloader.TryGetItem(identifier, out item) || item.Data == null) return;
			
			long contentLength = (long)item.Data;
			if (contentLength <= 0) return;
			string url = identifier.Substring(3);
			
			float contentLengthMB = (contentLength / 1024f / 1024f);
			warning.lines[3] = "Download size: " + contentLengthMB.ToString("F3") + " MB";
			warning.RedrawText();
		}
	}
}