// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.ComponentModel;
using System.Diagnostics;
using System.IO;
using System.Reflection;
using ClassicalSharp;
using OpenTK;

namespace Launcher {

	public static class Client {
		
		static DateTime lastJoin;
		public static bool Start(ClientStartData data, bool classicubeSkins, ref bool shouldExit) {
			if ((DateTime.UtcNow - lastJoin).TotalSeconds < 1)
				return false;
			lastJoin = DateTime.UtcNow;
			
			string skinServer = classicubeSkins ? "http://static.classicube.net/skins/" :
				"http://skins.minecraft.net/MinecraftSkins/";
			string args = data.Username + " " + data.Mppass + " " +
				data.Ip + " " + data.Port + " " + skinServer;
			return StartImpl(data, classicubeSkins, args, ref shouldExit);
		}
		
		public static bool Start(string args, ref bool shouldExit) {
			return StartImpl(null, true, args, ref shouldExit);
		}
		
		static bool StartImpl(ClientStartData data, bool classicubeSkins,
		                      string args, ref bool shouldExit) {
			string path = Path.Combine(Program.AppDirectory, "ClassicalSharp.exe");
			if (!File.Exists(path))
				return false;
			
			CheckSettings(data, classicubeSkins, out shouldExit);
			try {
				StartProcess(path, args);
			} catch (Win32Exception ex) {
				if ((uint)ex.ErrorCode != 0x80004005)
					throw; // HRESULT when user clicks 'cancel' to 'are you sure you want to run ClassicalSharp.exe'
				shouldExit = false;
				return false;
			}
			return true;
		}
		
		static void StartProcess(string path, string args) {
			if (Configuration.RunningOnMono) {
				// We also need to handle the case of running Mono through wine
				if (Configuration.RunningOnWindows) {
					try {
						Process.Start("mono", "\"" + path + "\" " + args);
					} catch (Win32Exception ex) {
						if (!((uint)ex.ErrorCode == 0x80070002 || (uint)ex.ErrorCode == 0x80004005))
							throw; // File not found HRESULT, HRESULT thrown when running on wine
						Process.Start(path, args);
					}
				} else if (Configuration.RunningOnMacOS && NeedOSXHack()) {
					Process.Start("mono", "--arch=32 \"" + path + "\" " + args);
				} else {
					Process.Start("mono", "\"" + path + "\" " + args);
				}
			} else {
				Process.Start(path, args);
			}
		}
		
		static bool NeedOSXHack() {
			Type type = Type.GetType("Mono.Runtime");
			if (type == null) return false;
			MethodInfo displayName = type.GetMethod("GetDisplayName", BindingFlags.NonPublic | BindingFlags.Static);
			if (displayName == null) return false;
			
			object versionRaw = displayName.Invoke(null, null);
			if (versionRaw == null) return false;
			
			string versionStr = versionRaw.ToString();
			if (versionStr.IndexOf(' ') >= 0) {
				versionStr = versionStr.Substring(0, versionStr.IndexOf(' '));
			}
			
			try {
				Version version = new Version(versionStr);
				return version.Major >= 5 && version.Minor >= 2;
			} catch {
				return false;
			}
		}
		
		internal static void CheckSettings(ClientStartData data, bool ccSkins, out bool shouldExit) {
			shouldExit = false;
			// Make sure if the client has changed some settings in the meantime, we keep the changes
			if (!Options.Load())
				return;
			shouldExit = Options.GetBool(OptionsKey.AutoCloseLauncher, false);
			if (data == null) return;
			
			Options.Set("launcher-server", data.Server);
			Options.Set("launcher-username", data.Username);
			Options.Set("launcher-ip", data.Ip);
			Options.Set("launcher-port", data.Port);
			Options.Set("launcher-mppass", Secure.Encode(data.Mppass, data.Username));
			Options.Set("launcher-ccskins", ccSkins);
			Options.Save();
		}
	}

	public class ClientStartData {
		public string Username, Mppass, Ip, Port, Server;
		
		public ClientStartData(string user, string mppass, string ip, string port, string server) {
			Username = user;
			Mppass = mppass;
			Ip = ip;
			Port = port;
			Server = server;
		}
	}
}
