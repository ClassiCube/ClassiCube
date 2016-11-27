// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
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
		InputWidget input;
		TextGroupWidget status, bottomRight, normalChat, clientStatus;
		bool suppressNextPress = true;
		int chatIndex;
		AltTextInputWidget altText;
		
		Font chatFont, chatUrlFont, announcementFont;
		public override void Init() {
			float textScale = game.Drawer2D.UseBitmappedChat ? 1.25f : 1;
			int fontSize = (int)(12 * game.GuiChatScale * textScale);
			Utils.Clamp(ref fontSize, 8, 60);
			chatFont = new Font(game.FontName, fontSize);
			chatUrlFont = new Font(game.FontName, fontSize, FontStyle.Underline);
			
			fontSize = (int)(14 * game.GuiChatScale);
			Utils.Clamp(ref fontSize, 8, 60);
			announcementFont = new Font(game.FontName, fontSize);
			ConstructWidgets();
			
			int[] indices = new int[chatLines];
			for (int i = 0; i < indices.Length; i++)
				indices[i] = -1;
			Metadata = indices;
			SetInitialMessages();
			
			game.Events.ChatReceived += ChatReceived;
			game.Events.ChatFontChanged += ChatFontChanged;
			game.Events.ColourCodeChanged += ColourCodeChanged;
		}
		
		void ConstructWidgets() {
			input = new ChatInputWidget(game, chatFont)
				.SetLocation(Anchor.LeftOrTop, Anchor.BottomOrRight, 5, 5);
			altText = new AltTextInputWidget(game, chatFont, input);
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
			for (int i = 0; i < chat.ClientStatus.Length; i++)
				clientStatus.SetText(i, chat.ClientStatus[i].Text);
			
			if (game.chatInInputBuffer != null) {
				OpenTextInputBar(game.chatInInputBuffer);
				game.chatInInputBuffer = null;
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
			Request item = game.AsyncDownloader.CurrentItem;
			if (item == null || !(item.Identifier == "terrain" || item.Identifier == "texturePack")) {
				if (status.Textures[1].IsValid) status.SetText(1, null);
				lastDownloadStatus = int.MinValue;
				return;
			}
			
			int progress = game.AsyncDownloader.CurrentItemProgress;
			if (progress == lastDownloadStatus) return;
			lastDownloadStatus = progress;
			SetFetchStatus(progress);
		}
		
		void SetFetchStatus(int progress) {
			lastDownload.Clear();
			int index = 0;
			if (progress == -2) {
				lastDownload.Append(ref index, "&eRetrieving texture pack..");
			} else if (progress == -1) {
				lastDownload.Append(ref index, "&eDownloading texture pack");
			} else if (progress >= 0 && progress <= 100) {
				lastDownload.Append(ref index, "&eDownloading texture pack (&7")
					.AppendNum(ref index, progress).Append(ref index, "&e%)");
			}
			status.SetText(1, lastDownload.ToString());
		}
		
		void RenderRecentChat(DateTime now, double delta) {
			int[] metadata = (int[])Metadata;
			for (int i = 0; i < normalChat.Textures.Length; i++) {
				Texture texture = normalChat.Textures[i];
				if (!texture.IsValid || metadata[i] == -1) continue;
				
				DateTime received = game.Chat.Log[metadata[i]].Received;
				if ((now - received).TotalSeconds <= 10)
					texture.Render(gfx);
			}
		}
		
		void RenderClientStatus() {
			int y = clientStatus.Y + clientStatus.Height;
			for (int i = 0; i < clientStatus.Textures.Length; i++) {
				Texture texture = clientStatus.Textures[i];
				if (!texture.IsValid) continue;
				
				y -= texture.Height;
				texture.Y1 = y;
				texture.Render(gfx);
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
			if (boxHeight > 0)
				gfx.Draw2DQuad(x, y, width, boxHeight + 10, backColour);
		}
		
		int inputOldHeight = -1;
		void UpdateChatYOffset(bool force) {
			int height = InputUsedHeight;
			if (force || height != inputOldHeight) {
				clientStatus.YOffset = Math.Max(hud.BottomOffset + 15, height);
				clientStatus.CalculatePosition();
				
				normalChat.YOffset = clientStatus.YOffset + clientStatus.GetUsedHeight();
				normalChat.CalculatePosition();
				inputOldHeight = height;
			}
		}

		void ColourCodeChanged(object sender, ColourCodeEventArgs e) {
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
			if (type == MessageType.Normal) {
				chatIndex++;
				if (game.ChatLines == 0) return;
				List<ChatLine> chat = game.Chat.Log;
				normalChat.PushUpAndReplaceLast(chat[chatIndex + chatLines - 1].Text);
				
				int[] metadata = (int[])Metadata;
				for (int i = 0; i < chatLines - 1; i++)
					metadata[i] = metadata[i + 1];
				metadata[chatLines - 1] = chatIndex + chatLines - 1;
			} else if (type >= MessageType.Status1 && type <= MessageType.Status3) {
				status.SetText(2 + (int)(type - MessageType.Status1), e.Text);
			} else if (type >= MessageType.BottomRight1 && type <= MessageType.BottomRight3) {
				bottomRight.SetText(2 - (int)(type - MessageType.BottomRight1), e.Text);
			} else if (type == MessageType.Announcement) {
				announcement.SetText(e.Text);
			} else if (type >= MessageType.ClientStatus1 && type <= MessageType.ClientStatus6) {
				clientStatus.SetText((int)(type - MessageType.ClientStatus1), e.Text);
				UpdateChatYOffset(true);
			}
		}

		public override void Dispose() {
			if (HandlesAllInput) {
				game.chatInInputBuffer = input.Text.ToString();
				game.CursorVisible = false;
			} else {
				game.chatInInputBuffer = null;
			}
			chatFont.Dispose();
			chatUrlFont.Dispose();
			announcementFont.Dispose();
			
			normalChat.Dispose();
			input.DisposeFully();
			altText.Dispose();
			status.Dispose();
			bottomRight.Dispose();
			clientStatus.Dispose();
			announcement.Dispose();
			
			game.Events.ChatReceived -= ChatReceived;
			game.Events.ChatFontChanged -= ChatFontChanged;
			game.Events.ColourCodeChanged -= ColourCodeChanged;
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
			int[] metadata = (int[])Metadata;
			for (int i = 0; i < chatLines; i++)
				metadata[i] = -1;
			
			for (int i = chatIndex; i < chatIndex + chatLines; i++) {
				if (i >= 0 && i < chat.Count) {
					normalChat.PushUpAndReplaceLast(chat[i].Text);
					metadata[i - chatIndex] = i;
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
			HandlesAllInput = true;
			game.Keyboard.KeyRepeat = true;
			
			input.Text.Clear();
			input.Text.Append(0, initialText);
			input.Init();
		}
		
		public void AppendTextToInput(string text) {
			if (!HandlesAllInput) return;
			input.Append(text);
		}
		
		public override bool HandlesKeyDown(Key key) {
			suppressNextPress = false;

			if (HandlesAllInput) { // text input bar
				if (key == game.Mapping(KeyBind.SendChat) || key == Key.KeypadEnter
				   || key == game.Mapping(KeyBind.PauseOrExit)) {
					HandlesAllInput = false;
					game.CursorVisible = false;
					game.Camera.RegrabMouse();
					game.Keyboard.KeyRepeat = false;
					
					if (key == game.Mapping(KeyBind.PauseOrExit))
						input.Clear();
					input.EnterInput();
					altText.SetActive(false);
					
					chatIndex = game.Chat.Log.Count - chatLines;
					ScrollHistory();
				} else if (key == Key.PageUp) {
					chatIndex -= chatLines;
					ScrollHistory();
				} else if (key == Key.PageDown) {
					chatIndex += chatLines;
					ScrollHistory();
				} else if (game.Server.SupportsFullCP437 &&
				          key == game.Input.Keys[KeyBind.ExtInput]) {
					altText.SetActive(!altText.Active);
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
		
		public override bool HandlesMouseScroll(int delta) {
			if (!HandlesAllInput) return false;
			chatIndex -= delta;
			ScrollHistory();
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
			if (new Rectangle(normalChat.X, y, normalChat.Width, height).Contains(mouseX, mouseY))
				return HandlesChatClick(mouseX, mouseY);
			return false;
		}
		
		bool HandlesChatClick(int mouseX, int mouseY) {
			string text = normalChat.GetSelected(mouseX, mouseY);
			if (text == null) return false;
			string url = Utils.StripColours(text);
			
			if (Utils.IsUrlPrefix(url, 0)) {
				WarningScreen warning = new WarningScreen(game, false, false);
				warning.Metadata = url;
				warning.SetHandlers(OpenUrl, AppendUrl, null);
				
				warning.SetTextData(
					"&eAre you sure you want to open this link?",
					url, "Be careful - links from strangers may be websites that",
					" have viruses, or things you may not want to open/see.");
				game.Gui.ShowWarning(warning);
			} else if (game.ClickableChat) {
				input.Append(text);
			}
			return true;
		}
		
		void OpenUrl(WarningScreen screen) {
			try {
				Process.Start((string)screen.Metadata);
			} catch(Exception ex) {
				ErrorHandler.LogError("ChatScreen.OpenUrl", ex);
			}
		}
		
		void AppendUrl(WarningScreen screen) {
			if (!game.ClickableChat) return;
			input.Append((string)screen.Metadata);
		}
		
		void ScrollHistory() {
			int maxIndex = game.Chat.Log.Count - chatLines;
			int minIndex = Math.Min(0, maxIndex);
			Utils.Clamp(ref chatIndex, minIndex, maxIndex);
			ResetChat();
		}
		
		int InputUsedHeight {
			get { return altText.Height == 0 ? input.Height + 20 : (game.Height - altText.Y + 5); }
		}
		
		void UpdateAltTextY() {
			int height = Math.Max(input.Height + input.YOffset, hud.BottomOffset);
			height += input.YOffset;
			altText.texture.Y1 = game.Height - (height + altText.texture.Height);
			altText.Y = altText.texture.Y1;
		}
	}
}