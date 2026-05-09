#pragma once

#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

// Forward declarations only — keeping <wrl/client.h> + <xaudio2.h> + the
// transitive <windows.h> drag entirely inside AudioEngine.cpp. Otherwise
// they leak the `min`/`max` macros into translation units that include this
// header (e.g. PlayerController.cpp's std::max calls would break).
struct IXAudio2;
struct IXAudio2MasteringVoice;
struct IXAudio2SourceVoice;
struct tWAVEFORMATEX;
typedef tWAVEFORMATEX WAVEFORMATEX;

// Minimal XAudio2 wrapper for placeholder game SFX. v1 capabilities:
//
//   - One mastering voice for output. Created at Init(); torn down by the
//     destructor.
//   - LoadSound(name, path) parses a 16-bit PCM WAV file (RIFF/WAVE/fmt/data
//     chunks) and stores both the format and the raw sample bytes against
//     `name`. Each loaded sound also owns one IXAudio2SourceVoice that we
//     reuse for every Play() call -- on Play we Stop + FlushSourceBuffers
//     + SubmitSourceBuffer + Start, so overlapping plays of the same SFX
//     restart rather than mix. Good enough for footstep-tier triggers.
//
//   - Play(name) is a no-op if the engine isn't initialized OR the sound
//     wasn't loaded -- we never want a missing asset to fail a hot path
//     in the gameplay loop.
//
// Future work (v2): voice pooling for true overlapping plays, OGG/MP3
// decode, per-channel volume + panning, BGM streaming.
class AudioEngine
{
public:
	AudioEngine();
	~AudioEngine();
	AudioEngine(const AudioEngine&) = delete;
	AudioEngine& operator=(const AudioEngine&) = delete;

	// Loads a 16-bit PCM WAV from disk and registers it under `name`.
	// Failure (bad file, unsupported format) is logged via OutputDebugString
	// and silently dropped -- the game keeps running with the missing slot
	// unmapped. Subsequent Play(name) calls are no-ops in that case.
	void LoadSound(const std::string& name, const std::filesystem::path& path);

	// Triggers playback of the named sound. Idempotent / safe to call from
	// any frame, including before Init succeeds. If the sound is already
	// playing, restarts from the head.
	void Play(const std::string& name);

private:
	struct Sound;
	void DestroySoundVoices();

	IXAudio2*                            xaudio_      = nullptr; // raw, ::Release in dtor
	IXAudio2MasteringVoice*              masterVoice_ = nullptr;
	std::unordered_map<std::string, Sound*> sounds_;
};
