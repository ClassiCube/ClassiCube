// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using System.Threading;
using SharpWave;

namespace ClassicalSharp.Audio {
	
	public sealed partial class AudioPlayer {
		
		Soundboard digBoard, stepBoard;
		const int maxSounds = 6;
		
		public void SetSounds(int volume) {
			if (volume > 0) InitSound();
			else DisposeSound();
		}
		
		void InitSound() {
			if (digBoard == null) InitSoundboards();
			monoOutputs = new IAudioOutput[maxSounds];
			stereoOutputs = new IAudioOutput[maxSounds];
		}
		
		void InitSoundboards() {
			digBoard = new Soundboard();
			digBoard.Init("dig_", files);
			stepBoard = new Soundboard();
			stepBoard.Init("step_", files);
		}

		void PlayBlockSound(object nill, BlockChangedEventArgs e) {
			if (e.Block == 0) {
				PlayDigSound(BlockInfo.DigSounds[e.OldBlock]);
			} else if (!game.ClassicMode) {
				PlayDigSound(BlockInfo.StepSounds[e.Block]);
			}
		}
		
		public void PlayDigSound(byte type) { PlaySound(type, digBoard); }
		
		public void PlayStepSound(byte type) { PlaySound(type, stepBoard); }
		
		AudioFormat format;
		AudioChunk chunk = new AudioChunk();
		void PlaySound(byte type, Soundboard board) {
			if (type == SoundType.None || monoOutputs == null) return;
			Sound snd = board.PickRandomSound(type);
			if (snd == null) return;
			
			format = snd.Format;
			chunk.Data = snd.Data;
			chunk.Length = snd.Data.Length;
			
			float volume = game.SoundsVolume / 100.0f;
			if (board == digBoard) {
				if (type == SoundType.Metal) format.SampleRate = (format.SampleRate * 6) / 5;
				else format.SampleRate = (format.SampleRate * 4) / 5;
			} else {
				volume *= 0.50f;
				if (type == SoundType.Metal) format.SampleRate = (format.SampleRate * 7) / 5;
			}
			
			if (format.Channels == 1) {
				PlayCurrentSound(monoOutputs, volume);
			} else if (format.Channels == 2) {
				PlayCurrentSound(stereoOutputs, volume);
			}
		}
		
		void PlayCurrentSound(IAudioOutput[] outputs, float volume) {
			for (int i = 0; i < outputs.Length; i++) {
				IAudioOutput output = outputs[i];
				if (output == null) {
					output = GetPlatformOut();
					output.Create(1);
					outputs[i] = output;
				}
				
				if (!output.IsFinished()) continue;				
				AudioFormat fmt = output.Format;
				if (fmt.Channels == 0 || fmt.Equals(format)) {
					PlaySound(output, volume); return;
				}
			}
			
			// This time we try to play the sound on all possible devices,
			// even if it requires the expensive case of recreating a device
			for (int i = 0; i < outputs.Length; i++) {
				IAudioOutput output = outputs[i];
				if (!output.IsFinished()) continue;
				
				PlaySound(output, volume); return;
			}
		}
		
		void PlaySound(IAudioOutput output, float volume) {
			try {
				output.SetVolume(volume);
				output.SetFormat(format);
				output.PlayData(0, chunk);
			} catch (InvalidOperationException ex) {
				ErrorHandler.LogError("AudioPlayer.PlayCurrentSound()", ex);
				if (ex.Message == "No audio devices found")
					game.Chat.Add("&cNo audio devices found, disabling sounds.");
				else
					game.Chat.Add("&cAn error occured when trying to play sounds, disabling sounds.");
				
				SetSounds(0);
				game.SoundsVolume = 0;
			}
		}
		
		void DisposeSound() {
			DisposeOutputs(ref monoOutputs);
			DisposeOutputs(ref stereoOutputs);
		}
		
		void DisposeOutputs(ref IAudioOutput[] outputs) {
			if (outputs == null) return;
			bool soundPlaying = true;
			
			while (soundPlaying) {
				soundPlaying = false;
				for (int i = 0; i < outputs.Length; i++) {
					if (outputs[i] == null) continue;
					soundPlaying |= !outputs[i].IsFinished();
				}
				if (soundPlaying)
					Thread.Sleep(1);
			}
			
			for (int i = 0; i < outputs.Length; i++) {
				if (outputs[i] == null) continue;
				outputs[i].Dispose();
			}
			outputs = null;
		}
	}
}
