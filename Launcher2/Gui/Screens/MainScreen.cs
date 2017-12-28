// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.Drawing;
using System.IO;
using System.Net;
using ClassicalSharp;
using Launcher.Gui.Views;
using Launcher.Web;
using Launcher.Gui.Widgets;

namespace Launcher.Gui.Screens {
	public sealed partial class MainScreen : InputScreen {
		
		MainView view;
		public MainScreen(LauncherWindow game) : base(game) {
			enterIndex = 2;
			view = new MainView(game);
			widgets = view.widgets;
			LoadResumeInfo();
		}
		
		public override void Init() {
			base.Init();
			view.Init();
			
			widgets[view.loginIndex].OnClick = LoginAsync;
			widgets[view.resIndex].OnClick = DoResume;
			widgets[view.dcIndex].OnClick = SwitchToDirectConnect;
			widgets[view.spIndex].OnClick = StartSingleplayer;
			widgets[view.settingsIndex].OnClick = SwitchToSettings;
			SetupInputHandlers();
			Resize();
			
			using (drawer) {
				drawer.SetBitmap(game.Framebuffer);
				LoadSavedInfo();
			}
		}
		
		public override void Resize() {
			view.DrawAll();
			game.Dirty = true;
		}
		
		void SwitchToDirectConnect(int x, int y) { game.SetScreen(new DirectConnectScreen(game)); }
		void StartSingleplayer(int x, int y) { Client.Start(widgets[0].Text, ref game.ShouldExit); }
		void SwitchToSettings(int x, int y) { game.SetScreen(new SettingsScreen(game)); }

		const int buttonWidth = 220, buttonHeight = 35, sideButtonWidth = 150;
		string resumeUser, resumeIp, resumePort, resumeMppass, resumeServer;
		bool resumeCCSkins, resumeValid;
		
		void DoResume(int mouseX, int mouseY) {
			if (!resumeValid) return;
			ClientStartData data = new ClientStartData(resumeUser, resumeMppass, resumeIp, resumePort, resumeServer);
			Client.Start(data, resumeCCSkins, ref game.ShouldExit);
		}
		
		
		void LoadResumeInfo() {
			resumeServer = Options.Get("launcher-server") ?? "";
			resumeUser = Options.Get("launcher-username");
			resumeIp = Options.Get("launcher-ip") ?? "";
			resumePort = Options.Get("launcher-port") ?? "";
			resumeCCSkins = Options.GetBool("launcher-ccskins", true);
			
			IPAddress address;
			if (!IPAddress.TryParse(resumeIp, out address)) resumeIp = "";
			ushort portNum;
			if (!UInt16.TryParse(resumePort, out portNum)) resumePort = "";
			
			string mppass = Options.Get("launcher-mppass") ?? null;
			resumeMppass = Secure.Decode(mppass, resumeUser);
			resumeValid = 
				!String.IsNullOrEmpty(resumeUser) && !String.IsNullOrEmpty(resumeIp) &&
				!String.IsNullOrEmpty(resumePort) && !String.IsNullOrEmpty(resumeMppass);
		}
		
		bool updateDone;
		void SuccessfulUpdateCheck(UpdateCheckTask task) {
			string latestVer = game.checkTask.LatestStable.Version.Substring(1);
			int spaceIndex = Program.AppName.LastIndexOf(' ');
			string currentVer = Program.AppName.Substring(spaceIndex + 1);
			bool update = new Version(latestVer) > new Version(currentVer);
			
			view.updateText = update ? "&aNew release" : "&eUp to date";
			game.RedrawBackground();
			Resize();
			SelectWidget(selectedWidget, 0, 0);
		}
		
		void FailedUpdateCheck(UpdateCheckTask task) {
			view.updateText = "&cCheck failed";
			game.RedrawBackground();
			Resize();
			SelectWidget(selectedWidget, 0, 0);
		}
		
		
		protected override void SelectWidget(Widget widget, int mouseX, int mouseY) {
			base.SelectWidget(widget, mouseX, mouseY);
			if (signingIn || !resumeValid || widget != widgets[view.resIndex]) return;
			
			string curUser = ((InputWidget)widgets[view.usernameIndex]).Text;
			if (resumeServer != "" && resumeUser == curUser) {
				SetStatus("&eResume to " + resumeServer);
			} else if (resumeServer != "") {
				SetStatus("&eResume as " + resumeUser + " to " + resumeServer);
			} else {
				SetStatus("&eResume as " + resumeUser + " to " + resumeIp + ":" + resumePort);
			}
			
		}
		
		protected override void UnselectWidget(Widget widget) {
			base.UnselectWidget(widget);
			if (signingIn || !resumeValid || widget != widgets[view.resIndex]) return;
			SetStatus("");
		}
		
		public override void Dispose() {
			StoreFields();
			base.Dispose();
		}
	}
}
