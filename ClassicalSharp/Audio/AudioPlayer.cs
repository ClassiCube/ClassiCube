// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using System.IO;
using System.Threading;
using SharpWave;
using SharpWave.Codecs.Vorbis;

namespace ClassicalSharp.Audio {
	
	public sealed partial class AudioPlayer : IGameComponent {
		
		IAudioOutput musicOut;
		IAudioOutput[] monoOutputs, stereoOutputs;
		string[] files, musicFiles;
		Thread musicThread;
		Game game;
		
		public void Init(Game game) {
			this.game = game;
			string path = Path.Combine(Program.AppDirectory, "audio");
			if (Directory.Exists(path))
				files = Directory.GetFiles(path);
			else
				files = new string[0];
						
			game.MusicVolume = GetVolume(OptionsKey.MusicVolume, OptionsKey.UseMusic);
			SetMusic(game.MusicVolume);
			game.SoundsVolume = GetVolume(OptionsKey.SoundsVolume, OptionsKey.UseSound);
			SetSounds(game.SoundsVolume);
			game.UserEvents.BlockChanged += PlayBlockSound;
		}
		
		static int GetVolume(string volKey, string boolKey) {
			int volume = Options.GetInt(volKey, 0, 100, 0);
			if (volume != 0) return volume;
			
			volume = Options.GetBool(boolKey, false) ? 100 : 0;
			Options.Set<string>(boolKey, null);
			return volume;
		}

		public void Ready(Game game) { }
		public void Reset(Game game) { }
		public void OnNewMap(Game game) { }
		public void OnNewMapLoaded(Game game) { }
		
		public void SetMusic(int volume) {
			if (volume > 0) InitMusic();
			else DisposeMusic();
		}
		
		const StringComparison comp = StringComparison.OrdinalIgnoreCase;
		void InitMusic() {
			if (musicThread != null) { musicOut.SetVolume(game.MusicVolume / 100.0f); return; }
			
			int musicCount = 0;
			for (int i = 0; i < files.Length; i++) {
				if (files[i].EndsWith(".ogg", comp)) musicCount++;
			}
			
			musicFiles = new string[musicCount];
			for (int i = 0, j = 0; i < files.Length; i++) {
				if (!files[i].EndsWith(".ogg", comp)) continue;
				musicFiles[j] = files[i]; j++;
			}

			disposingMusic = false;
			musicOut = GetPlatformOut();
			musicOut.Create(10);
			musicThread = MakeThread(DoMusicThread, "ClassicalSharp.DoMusic");
		}
		
		EventWaitHandle musicHandle = new EventWaitHandle(false, EventResetMode.AutoReset);
		void DoMusicThread() {
			if (musicFiles.Length == 0) return;
			Random rnd = new Random();
			while (!disposingMusic) {
				string file = musicFiles[rnd.Next(0, musicFiles.Length)];
				string path = Path.Combine(Program.AppDirectory, file);
				Utils.LogDebug("playing music file: " + file);
				
				using (FileStream fs = File.OpenRead(path)) {
					OggContainer container = new OggContainer(fs);
					try {
						musicOut.SetVolume(game.MusicVolume / 100.0f);
						musicOut.PlayStreaming(container);
					} catch (InvalidOperationException ex) {
						HandleMusicError(ex);
						return;
					} catch (Exception ex) {
						ErrorHandler.LogError("AudioPlayer.DoMusicThread()", ex);
						game.Chat.Add("&cError while trying to play music file " + Path.GetFileName(file));
					}
				}
				if (disposingMusic) break;
				
				int delay = 2000 * 60 + rnd.Next(0, 5000 * 60);
				musicHandle.WaitOne(delay, false);
			}
		}
		
		void HandleMusicError(InvalidOperationException ex) {
			ErrorHandler.LogError("AudioPlayer.DoMusicThread()", ex);
			if (ex.Message == "No audio devices found")
				game.Chat.Add("&cNo audio devices found, disabling music.");
			else
				game.Chat.Add("&cAn error occured when trying to play music, disabling music.");
			
			SetMusic(0);
			game.MusicVolume = 0;
		}
		
		bool disposingMusic;
		public void Dispose() {
			DisposeMusic();
			DisposeSound();
			musicHandle.Close();
			game.UserEvents.BlockChanged -= PlayBlockSound;
		}
		
		void DisposeMusic() {
			disposingMusic = true;
			musicHandle.Set();
			DisposeOf(ref musicOut, ref musicThread);
		}
		
		Thread MakeThread(ThreadStart func, string name) {
			Thread thread = new Thread(func);
			thread.Name = name;
			thread.IsBackground = true;
			thread.Start();
			return thread;
		}
		
		IAudioOutput GetPlatformOut() {
			if (OpenTK.Configuration.RunningOnWindows && !Options.GetBool(OptionsKey.ForceOpenAL, false))
				return new WinMmOut();
			return new OpenALOut();
		}
		
		void DisposeOf(ref IAudioOutput output, ref Thread thread) {
			if (output == null) return;
			output.Stop();
			thread.Join();
			
			output.Dispose();
			output = null;
			thread = null;
		}
	}
}
