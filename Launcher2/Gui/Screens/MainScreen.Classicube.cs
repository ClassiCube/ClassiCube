// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.Collections.Generic;
using System.Net;
using ClassicalSharp;
using Launcher.Gui.Widgets;
using Launcher.Web;

namespace Launcher.Gui.Screens {
	public sealed partial class MainScreen : InputScreen {
		
		GetCSRFTokenTask getTask;
		SignInTask postTask;
		FetchServersTask fetchTask;
		bool signingIn = false;
		
		public override void Tick() {
			base.Tick();
			if (game.checkTask != null && game.checkTask.Completed && !updateDone) {
				bool success = game.checkTask.Success;
				if (success) SuccessfulUpdateCheck(game.checkTask);
				else FailedUpdateCheck(game.checkTask);
				updateDone = true;
			}
			if (!signingIn) return;
			
			if (getTask != null) {
				LoginGetTick();
			} else if (postTask != null) {
				LoginPostTick();
			} else if (fetchTask != null) {
				FetchTick();
			}
		}
		
		void LoginGetTick() {
			getTask.Tick();
			if (!getTask.Completed) return;
			
			if (getTask.Success) {
				postTask = new SignInTask();
				postTask.Username = Get(0);
				postTask.Password = Get(1);
				postTask.Token = getTask.Token;
				postTask.RunAsync(game);
			} else {
				DisplayWebException(getTask.WebEx, "sign in");
			}
			
			getTask = null;
			game.RedrawBackground();
			Resize();
		}
		
		void LoginPostTick() {
			postTask.Tick();
			if (!postTask.Completed) return;
			
			if (postTask.Error != null) {
				SetStatus("&c" + postTask.Error);
			} else if (postTask.Success) {
				game.Username = postTask.Username;
				fetchTask = new FetchServersTask();
				fetchTask.RunAsync(game);
				SetStatus("&eRetrieving servers list..");
			} else {
				DisplayWebException(postTask.WebEx, "sign in");
			}
			
			postTask = null;
			game.RedrawBackground();
			Resize();
		}
		
		void FetchTick() {
			fetchTask.Tick();
			if (!fetchTask.Completed) return;
			
			if (fetchTask.Success) {
				game.Servers = fetchTask.Servers;
				game.SetScreen(new ServersScreen(game));
			} else {
				DisplayWebException(fetchTask.WebEx, "retrieving servers list");
				game.RedrawBackground();
				Resize();
			}
			
			fetchTask = null;
		}

		string lastStatus;
		void SetStatus(string text) {
			lastStatus = text;
			LabelWidget widget = (LabelWidget)widgets[view.statusIndex];
			
			game.ResetArea(widget.X, widget.Y, widget.Width, widget.Height);
			widget.SetDrawData(drawer, text);
			RedrawWidget(widget);
			game.Dirty = true;
		}

		void LoginAsync(int mouseX, int mouseY) {
			if (String.IsNullOrEmpty(Get(0))) {
				SetStatus("&eUsername required"); return;
			}
			if (String.IsNullOrEmpty(Get(1))) {
				SetStatus("&ePassword required"); return;
			}
			if (getTask != null) return;
			
			game.Username = Get(0);
			UpdateSignInInfo(Get(0), Get(1));
			
			CheckboxWidget skip = widgets[view.sslIndex] as CheckboxWidget;
			if (skip != null && skip.Value) {
				ServicePointManager.ServerCertificateValidationCallback = delegate { return true; };
				Options.Set("skip-ssl-check", true);
			} else {
				ServicePointManager.ServerCertificateValidationCallback = null;
			}
			
			getTask = new GetCSRFTokenTask();
			getTask.RunAsync(game);
			game.RedrawBackground();
			Resize();
			
			SetStatus("&eSigning in..");
			signingIn = true;
		}

		void DisplayWebException(WebException ex, string action) {
			ErrorHandler.LogError(action, ex);
			bool sslCertError = ex.Status == WebExceptionStatus.TrustFailure ||
				(ex.Status == WebExceptionStatus.SendFailure && OpenTK.Configuration.RunningOnMono);
			signingIn = false;
			
			if (ex.Status == WebExceptionStatus.Timeout) {
				string text = "&cTimed out when connecting to classicube.net.";
				SetStatus(text);
			} else if (ex.Status == WebExceptionStatus.ProtocolError) {
				HttpWebResponse response = (HttpWebResponse)ex.Response;
				int errorCode = (int)response.StatusCode;
				string description = response.StatusDescription;
				string text = "&cclassicube.net returned: (" + errorCode + ") " + description;
				SetStatus(text);
			} else if (ex.Status == WebExceptionStatus.NameResolutionFailure) {
				string text = "&cUnable to resolve classicube.net" +
					Environment.NewLine + "Are you connected to the internet?";
				SetStatus(text);
			} else if (sslCertError) {
				string text = "&cFailed to validate SSL certificate";
				SetStatus(text);
				ShowSSLErrorWidgets();
			} else {
				string text = "&c" + ex.Status;
				SetStatus(text);
			}
		}
		
		void ShowSSLErrorWidgets() {
			using (drawer) {
				drawer.SetBitmap(game.Framebuffer);
				widgets[view.sslIndex].Visible = true;
				widgets[view.sslIndex + 1].Visible = true;
				
				widgets[view.sslIndex].OnClick = SSLSkipValidationClick;
				widgets[view.sslIndex].Redraw(drawer);
				widgets[view.sslIndex + 1].Redraw(drawer);
			}
		}
		
		void SSLSkipValidationClick(int mouseX, int mouseY) {
			using (drawer) {
				drawer.SetBitmap(game.Framebuffer);
				CheckboxWidget widget = (CheckboxWidget)widgets[view.sslIndex];
				SetBool(!widget.Value);
			}
		}
		
		void SetBool(bool value) {
			CheckboxWidget widget = (CheckboxWidget)widgets[view.sslIndex];
			widget.Value = value;
			widget.Redraw(game.Drawer);
			game.Dirty = true;
		}
		
		static string cachedUser, cachedPass;
		void StoreFields() {
			cachedUser = Get(0);
			cachedPass = Get(1);
		}
		
		void LoadSavedInfo() {
			// restore what user last typed into the various fields
			if (cachedUser != null) {
				Set(0, cachedUser);
				Set(1, cachedPass);
			} else {
				LoadFromOptions();
			}
		}
		
		void LoadFromOptions() {
			if (!Options.Load())
				return;
			
			string user = Options.Get("launcher-cc-username", "");
			string pass = Options.Get("launcher-cc-password", "");
			pass = Secure.Decode(pass, user);
			
			Set(0, user);
			Set(1, pass);
		}
		
		void UpdateSignInInfo(string user, string password) {
			// If the client has changed some settings in the meantime, make sure we keep the changes
			if (!Options.Load()) return;
			
			Options.Set("launcher-cc-username", user);
			Options.Set("launcher-cc-password", Secure.Encode(password, user));
			Options.Save();
		}
	}
}
