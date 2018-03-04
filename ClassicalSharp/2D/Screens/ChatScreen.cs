// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Drawing;
using ClassicalSharp.Events;
using ClassicalSharp.Gui.Widgets;
using ClassicalSharp.Network;
using OpenTK.Input;

namespace ClassicalSharp.Gui.Screens {
	public class ChatScreen : Screen {
		
		public ChatScreen(Game game, HudScreen hud) : base(game) {
			chatLines = game.ChatLines;
			this.hud = hud;
		}
		
		HudScreen hud;
		int chatLines;
		TextWidget announcement;
		internal InputWidget input;
		TextGroupWidget status, bottomRight, normalChat, clientStatus;
		bool suppressNextPress = true;
		int chatIndex;
		SpecialInputWidget altText;
		
		Font chatFont, chatUrlFont, announcementFont;
		// needed for lost contexts, to restore chat typed in
		static string chatInInputBuffer = null;
		
		public override void Init() {
			int fontSize = (int)(8 * game.GuiChatScale);
			Utils.Clamp(ref fontSize, 8, 60);
			chatFont = new Font(game.FontName, fontSize);
			chatUrlFont = new Font(game.FontName, fontSize, FontStyle.Underline);
			
			fontSize = (int)(16 * game.GuiChatScale);
			Utils.Clamp(ref fontSize, 8, 60);
			announcementFont = new Font(game.FontName, fontSize);
			ContextRecreated();
			
			game.Events.ChatReceived += ChatReceived;
			game.Events.ChatFontChanged += ChatFontChanged;
			game.Events.ColourCodeChanged += ColourCodeChanged;
			game.Graphics.ContextLost += ContextLost;
			game.Graphics.ContextRecreated += ContextRecreated;
		}
		
		void ConstructWidgets() {
			input = new ChatInputWidget(game, chatFont)
				.SetLocation(Anchor.LeftOrTop, Anchor.BottomOrRight, 5, 5);
			altText = new SpecialInputWidget(game, chatFont, input);
			altText.Init();
			UpdateAltTextY();
			
			status = new TextGroupWidget(game, 5, chatFont, chatUrlFont)
				.SetLocation(Anchor.BottomOrRight, Anchor.LeftOrTop, 0, 0);
			status.Init();
			status.SetUsePlaceHolder(0, false);
			status.SetUsePlaceHolder(1, false);
			
			bottomRight = new TextGroupWidget(game, 3, chatFont, chatUrlFont)
				.SetLocation(Anchor.BottomOrRight, Anchor.BottomOrRight, 0, hud.BottomOffset + 15);
			bottomRight.Init();
			
			normalChat = new TextGroupWidget(game, chatLines, chatFont, chatUrlFont)
				.SetLocation(Anchor.LeftOrTop, Anchor.BottomOrRight, 10, hud.BottomOffset + 15);
			normalChat.Init();
			
			clientStatus = new TextGroupWidget(game, game.Chat.ClientStatus.Length, chatFont, chatUrlFont)
				.SetLocation(Anchor.LeftOrTop, Anchor.BottomOrRight, 10, hud.BottomOffset + 15);
			clientStatus.Init();
			
			announcement = TextWidget.Create(game ,null, announcementFont)
				.SetLocation(Anchor.Centre, Anchor.Centre, 0, -game.Height / 4);
		}
		
		void SetInitialMessages() {
			Chat chat = game.Chat;
			chatIndex = chat.Log.Count - chatLines;
			ResetChat();
			status.SetText(2, chat.Status1.Text);
			status.SetText(3, chat.Status2.Text);
			status.SetText(4, chat.Status3.Text);
			
			bottomRight.SetText(2, chat.BottomRight1.Text);
			bottomRight.SetText(1, chat.BottomRight2.Text);
			bottomRight.SetText(0, chat.BottomRight3.Text);
			announcement.SetText(chat.Announcement.Text);
			for (int i = 0; i < chat.ClientStatus.Length; i++) {
				clientStatus.SetText(i, chat.ClientStatus[i].Text);
			}
			
			if (chatInInputBuffer != null) {
				OpenTextInputBar(chatInInputBuffer);
				chatInInputBuffer = null;
			}
		}
		
		public override void Render(double delta) {
			if (!game.PureClassic)
				status.Render(delta);
			bottomRight.Render(delta);
			CheckOtherStatuses();
			
			UpdateChatYOffset(false);
			RenderClientStatus();
			DateTime now = DateTime.UtcNow;
			if (HandlesAllInput) {
				normalChat.Render(delta);
			} else {
				RenderRecentChat(now, delta);
			}
			announcement.Render(delta);
			
			if (HandlesAllInput) {
				input.Render(delta);
				if (altText.Active)
					altText.Render(delta);
			}
			
			if (announcement.IsValid && (now - game.Chat.Announcement.Received).TotalSeconds > 5)
				announcement.Dispose();
		}
		
		int lastDownloadStatus = int.MinValue;
		StringBuffer lastDownload = new StringBuffer(48);
		void CheckOtherStatuses() {
			Request item = game.Downloader.CurrentItem;
			if (item == null || !(item.Identifier == "terrain" || item.Identifier == "texturePack")) {
				if (status.Textures[1].IsValid) status.SetText(1, null);
				lastDownloadStatus = int.MinValue;
				return;
			}
			
			int progress = game.Downloader.CurrentItemProgress;
			if (progress == lastDownloadStatus) return;
			lastDownloadStatus = progress;
			SetFetchStatus(progress);
		}
		
		void SetFetchStatus(int progress) {
			lastDownload.Clear();
			if (progress == -2) {
				lastDownload.Append("&eRetrieving texture pack..");
			} else if (progress == -1) {
				lastDownload.Append("&eDownloading texture pack");
			} else if (progress >= 0 && progress <= 100) {
				lastDownload.Append("&eDownloading texture pack (&7")
					.AppendNum(progress).Append("&e%)");
			}
			status.SetText(1, lastDownload.ToString());
		}
		
		void RenderRecentChat(DateTime now, double delta) {
			for (int i = 0; i < normalChat.Textures.Length; i++) {
				Texture texture = normalChat.Textures[i];
				int logIdx = chatIndex + i;
				
				if (!texture.IsValid) continue;
				if (logIdx < 0 || logIdx >= game.Chat.Log.Count) continue;
				
				DateTime received = game.Chat.Log[logIdx].Received;
				if ((now - received).TotalSeconds <= 10) {
					texture.Render(game.Graphics);
				}
			}
		}
		
		void RenderClientStatus() {
			int y = clientStatus.Y + clientStatus.Height;
			for (int i = 0; i < clientStatus.Textures.Length; i++) {
				Texture texture = clientStatus.Textures[i];
				if (!texture.IsValid) continue;
				
				y -= texture.Height;
				texture.Y1 = y;
				texture.Render(game.Graphics);
			}
		}
		
		static FastColour backColour = new FastColour(0, 0, 0, 127);
		public void RenderBackground() {
			int minIndex = Math.Min(0, game.Chat.Log.Count - chatLines);
			int height = chatIndex == minIndex ? normalChat.GetUsedHeight() : normalChat.Height;
			
			int y = normalChat.Y + normalChat.Height - height - 5;
			int x = normalChat.X - 5;
			int width = Math.Max(clientStatus.Width, normalChat.Width) + 10;
			
			int boxHeight = height + clientStatus.GetUsedHeight();
			if (boxHeight > 0) {
				game.Graphics.Draw2DQuad(x, y, width, boxHeight + 10, backColour);
			}
		}
		
		int inputOldHeight = -1;
		void UpdateChatYOffset(bool force) {
			int height = InputUsedHeight;
			if (force || height != inputOldHeight) {
				clientStatus.YOffset = Math.Max(hud.BottomOffset + 15, height);
				clientStatus.Reposition();
				
				normalChat.YOffset = clientStatus.YOffset + clientStatus.GetUsedHeight();
				normalChat.Reposition();
				inputOldHeight = height;
			}
		}

		void ColourCodeChanged(object sender, ColourCodeEventArgs e) {
			if (game.Graphics.LostContext) return;
			
			altText.UpdateColours();
			Recreate(normalChat, e.Code); Recreate(status, e.Code);
			Recreate(bottomRight, e.Code); Recreate(clientStatus, e.Code);
			input.Recreate();
		}
		
		void Recreate(TextGroupWidget group, char code) {
			for (int i = 0; i < group.lines.Length; i++) {
				string line = group.lines[i];
				if (line == null) continue;
				
				for (int j = 0; j < line.Length - 1; j++) {
					if (line[j] == '&' && line[j + 1] == code) {
						group.SetText(i, line); break;
					}
				}
			}
		}

		void ChatReceived(object sender, ChatEventArgs e) {
			MessageType type = e.Type;
			if (game.Graphics.LostContext) return;
			
			if (type == MessageType.Normal) {
				chatIndex++;
				if (game.ChatLines == 0) return;
				
				List<ChatLine> chat = game.Chat.Log;
				normalChat.PushUpAndReplaceLast(chat[chatIndex + chatLines - 1].Text);

			} else if (type >= MessageType.Status1 && type <= MessageType.Status3) {
				status.SetText(2 + (int)(type - MessageType.Status1), e.Text);
			} else if (type >= MessageType.BottomRight1 && type <= MessageType.BottomRight3) {
				bottomRight.SetText(2 - (int)(type - MessageType.BottomRight1), e.Text);
			} else if (type == MessageType.Announcement) {
				announcement.SetText(e.Text);
			} else if (type >= MessageType.ClientStatus1 && type <= MessageType.ClientStatus3) {
				clientStatus.SetText((int)(type - MessageType.ClientStatus1), e.Text);
				UpdateChatYOffset(true);
			}
		}

		public override void Dispose() {
			ContextLost();
			chatFont.Dispose();
			chatUrlFont.Dispose();
			announcementFont.Dispose();
			
			game.Events.ChatReceived -= ChatReceived;
			game.Events.ChatFontChanged -= ChatFontChanged;
			game.Events.ColourCodeChanged -= ColourCodeChanged;
			game.Graphics.ContextLost -= ContextLost;
			game.Graphics.ContextRecreated -= ContextRecreated;
		}
		
		protected override void ContextLost() {
			if (HandlesAllInput) {
				chatInInputBuffer = input.Text.ToString();
				game.CursorVisible = false;
			} else {
				chatInInputBuffer = null;
			}

			normalChat.Dispose();
			input.Dispose();
			altText.Dispose();
			status.Dispose();
			bottomRight.Dispose();
			clientStatus.Dispose();
			announcement.Dispose();
		}
		
		protected override void ContextRecreated() {
			ConstructWidgets();
			SetInitialMessages();
		}
		
		void ChatFontChanged(object sender, EventArgs e) {
			if (!game.Drawer2D.UseBitmappedChat) return;
			Recreate();
			UpdateChatYOffset(true);
		}
		
		public override void OnResize(int width, int height) {
			bool active = altText != null && altText.Active;
			Recreate();
			altText.SetActive(active);
		}

		void ResetChat() {
			normalChat.Dispose();
			List<ChatLine> chat = game.Chat.Log;
			
			for (int i = chatIndex; i < chatIndex + chatLines; i++) {
				if (i >= 0 && i < chat.Count) {
					normalChat.PushUpAndReplaceLast(chat[i].Text);
				}
			}
		}
		
		public override bool HandlesKeyPress(char key) {
			if (!HandlesAllInput) return false;
			if (suppressNextPress) {
				suppressNextPress = false;
				return false;
			}
			
			bool handled = input.HandlesKeyPress(key);
			UpdateAltTextY();
			return handled;
		}
		
		public void OpenTextInputBar(string initialText) {
			game.CursorVisible = true;
			suppressNextPress = true;
			SetHandlesAllInput(true);
			game.Keyboard.KeyRepeat = true;
			
			input.Text.Clear();
			input.Text.Set(initialText);
			input.Recreate();
		}
		
		public void AppendTextToInput(string text) {
			if (!HandlesAllInput) return;
			input.Append(text);
		}
		
		public override bool HandlesKeyDown(Key key) {
			suppressNextPress = false;
			if (HandlesAllInput) { // text input bar
				if (key == game.Mapping(KeyBind.SendChat) || key == Key.KeypadEnter || key == game.Mapping(KeyBind.PauseOrExit)) {
					SetHandlesAllInput(false);
					game.CursorVisible = false;
					game.Camera.RegrabMouse();
					game.Keyboard.KeyRepeat = false;
					
					if (key == game.Mapping(KeyBind.PauseOrExit))
						input.Clear();
					input.EnterInput();
					altText.SetActive(false);
					
					// Do we need to move all chat down?
					int resetIndex = game.Chat.Log.Count - chatLines;
					if (chatIndex != resetIndex) {
						chatIndex = ClampIndex(resetIndex);
						ResetChat();
					}
				} else if (key == Key.PageUp) {
					ScrollHistoryBy(-chatLines);
				} else if (key == Key.PageDown) {
					ScrollHistoryBy(chatLines);
				} else {
					input.HandlesKeyDown(key);
					UpdateAltTextY();
				}
				return key < Key.F1 || key > Key.F35;
			}

			if (key == game.Mapping(KeyBind.Chat)) {
				OpenTextInputBar("");
			} else if (key == Key.Slash) {
				OpenTextInputBar("/");
			} else {
				return false;
			}
			return true;
		}
		
		public override bool HandlesKeyUp(Key key) {
			if (!HandlesAllInput) return false;
			
			if (game.Server.SupportsFullCP437 && key == game.Input.Keys[KeyBind.ExtInput]) {
				if (game.window.Focused) altText.SetActive(!altText.Active);
			}
			return true;
		}
		
		float chatAcc;
		public override bool HandlesMouseScroll(float delta) {
			if (!HandlesAllInput) return false;
			int steps = Utils.AccumulateWheelDelta(ref chatAcc, delta);
			ScrollHistoryBy(-steps);
			return true;
		}
		
		public override bool HandlesMouseClick(int mouseX, int mouseY, MouseButton button) {
			if (!HandlesAllInput || game.HideGui) return false;
			
			if (!normalChat.Bounds.Contains(mouseX, mouseY)) {
				if (altText.Active && altText.Bounds.Contains(mouseX, mouseY)) {
					altText.HandlesMouseClick(mouseX, mouseY, button);
					UpdateAltTextY();
					return true;
				}
				input.HandlesMouseClick(mouseX, mouseY, button);
				return true;
			}
			
			int height = normalChat.GetUsedHeight();
			int y = normalChat.Y + normalChat.Height - height;
			if (GuiElement.Contains(normalChat.X, y, normalChat.Width, height, mouseX, mouseY))
				return HandlesChatClick(mouseX, mouseY);
			return false;
		}
		
		bool HandlesChatClick(int mouseX, int mouseY) {
			string text = normalChat.GetSelected(mouseX, mouseY);
			if (text == null) return false;
			string url = Utils.StripColours(text);
			
			if (Utils.IsUrlPrefix(url, 0)) {
				WarningOverlay overlay = new WarningOverlay(game, false, false);
				overlay.Metadata = url;
				overlay.SetHandlers(OpenUrl, AppendUrl);
				overlay.lines[0] = "&eAre you sure you want to open this link?";
				
				overlay.lines[1] = url;
				overlay.lines[2] = "Be careful - links from strangers may be websites that";
				overlay.lines[3] = " have viruses, or things you may not want to open/see.";
				game.Gui.ShowOverlay(overlay);
			} else if (game.ClickableChat) {
				input.Append(text);
			}
			return true;
		}
		
		void OpenUrl(Overlay urlOverlay, bool always) {
			try {
				Process.Start(urlOverlay.Metadata);
			} catch (Exception ex) {
				ErrorHandler.LogError("ChatScreen.OpenUrl", ex);
			}
		}
		
		void AppendUrl(Overlay urlOverlay, bool always) {
			if (!game.ClickableChat) return;
			input.Append(urlOverlay.Metadata);
		}
		
		int ClampIndex(int index) {
			int maxIndex = game.Chat.Log.Count - chatLines;
			int minIndex = Math.Min(0, maxIndex);
			Utils.Clamp(ref index, minIndex, maxIndex);
			return index;
		}
		
		void ScrollHistoryBy(int delta) {
			int newIndex = ClampIndex(chatIndex + delta);
			if (newIndex == chatIndex) return;
			
			chatIndex = newIndex;
			ResetChat();
		}
		
		int InputUsedHeight {
			get { return altText.Height == 0 ? input.Height + 20 : (game.Height - altText.Y + 5); }
		}
		
		void UpdateAltTextY() {
			int height = Math.Max(input.Height + input.YOffset, hud.BottomOffset);
			height += input.YOffset;
			altText.texture.Y1 = game.Height - (height + altText.texture.Height);
			altText.Y = altText.texture.Y;
		}
		
		void SetHandlesAllInput(bool handles) {
			HandlesAllInput = handles;
			game.Gui.hudScreen.HandlesAllInput = handles;
		}
	}
}