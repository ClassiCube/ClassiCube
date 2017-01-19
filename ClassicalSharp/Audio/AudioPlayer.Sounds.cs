// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using System.Threading;
using ClassicalSharp.Events;
using SharpWave;
using SharpWave.Codecs;

namespace ClassicalSharp.Audio {
	
	public sealed partial class AudioPlayer {
		
		Soundboard digBoard, stepBoard;
		const int maxSounds = 6;
		
		public void SetSound(bool enabled) {
			if (enabled)
				InitSound();
			else
				DisposeSound();
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

		void PlayBlockSound(object sender, BlockChangedEventArgs e) {
			if (e.Block == 0)
				PlayDigSound(game.BlockInfo.DigSounds[e.OldBlock]);
			else
				PlayDigSound(game.BlockInfo.StepSounds[e.Block]);
		}
		
		public void PlayDigSound(SoundType type) { PlaySound(type, digBoard); }
		
		public void PlayStepSound(SoundType type) { PlaySound(type, stepBoard); }
		
		AudioChunk chunk = new AudioChunk();
		void PlaySound(SoundType type, Soundboard board) {
			if (type == SoundType.None || monoOutputs == null)
				return;
			Sound snd = board.PickRandomSound(type);
			if (snd == null) return;
			
			chunk.Channels = snd.Channels;
			chunk.Frequency = snd.SampleRate;
			chunk.BitsPerSample = snd.BitsPerSample;
			chunk.BytesOffset = 0;
			chunk.BytesUsed = snd.Data.Length;
			chunk.Data = snd.Data;
			
			if (snd.Channels == 1) {
				PlayCurrentSound(monoOutputs);
			} else if (snd.Channels == 2) {
				PlayCurrentSound(stereoOutputs);
			}
		}
		
		IAudioOutput firstSoundOut;
		void PlayCurrentSound(IAudioOutput[] outputs) {
			for (int i = 0; i < monoOutputs.Length; i++) {
				IAudioOutput output = outputs[i];
				if (output == null) {
					output = GetPlatformOut();
					output.Create(1, firstSoundOut);
					if (firstSoundOut == null)
						firstSoundOut = output;
					outputs[i] = output;
				}
				if (!output.DoneRawAsync()) continue;
				
				try {
					output.PlayRawAsync(chunk);
				} catch (InvalidOperationException ex) {
					HandleSoundError(ex);
				}
				return;
			}
		}
		
		void HandleSoundError(InvalidOperationException ex) {
			ErrorHandler.LogError("AudioPlayer.PlayCurrentSound()", ex);
			if (ex.Message == "No audio devices found")
				game.Chat.Add("&cNo audio devices found, disabling sounds.");
			else
				game.Chat.Add("&cAn error occured when trying to play sounds, disabling sounds.");
			
			SetSound(false);
			game.UseSound = false;
		}
		
		void DisposeSound() {
			DisposeOutputs(ref monoOutputs);
			DisposeOutputs(ref stereoOutputs);
			if (firstSoundOut != null) {
				firstSoundOut.Dispose();
				firstSoundOut = null;
			}
		}
		
		void DisposeOutputs(ref IAudioOutput[] outputs) {
			if (outputs == null) return;
			bool soundPlaying = true;
			
			while (soundPlaying) {
				soundPlaying = false;
				for (int i = 0; i < outputs.Length; i++) {
					if (outputs[i] == null) continue;
					soundPlaying |= !outputs[i].DoneRawAsync();
				}
				if (soundPlaying)
					Thread.Sleep(1);
			}
			
			for (int i = 0; i < outputs.Length; i++) {
				if (outputs[i] == null || outputs[i] == firstSoundOut) continue;
				outputs[i].Dispose();
			}
			outputs = null;
		}
	}
}