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

// Minimal XAudio2 wrapper for placeholder game SFX. v2 capabilities:
//
//   - One mastering voice for output. Created at Init(); torn down by the
//     destructor.
//   - LoadSound(name, path) parses a 16-bit PCM WAV file. Each sound
//     owns a POOL of IXAudio2SourceVoice instances (kSfxPoolSize, default
//     4) so overlapping triggers play simultaneously instead of restarting
//     the same voice. Play() picks the next voice round-robin; if all
//     pool slots happen to still be active, the oldest is reused.
//   - LoadBgm(name, path) is the streaming-style counterpart: a single
//     looping voice configured with XAUDIO2_LOOP_INFINITE for background
//     music tracks. Only one BGM may be active at a time; PlayBgm()
//     stops any previous BGM and starts the named one.
//   - Play(name) and PlayBgm(name) are no-ops if the engine isn't
//     initialized or the sound wasn't loaded.
//
// Future work: OGG/MP3 decode, master/sfx/bgm submix volume buses,
// 3D-positional audio.
class AudioEngine
{
public:
	AudioEngine();
	~AudioEngine();
	AudioEngine(const AudioEngine&) = delete;
	AudioEngine& operator=(const AudioEngine&) = delete;

	// Loads a 16-bit PCM WAV from disk and registers it under `name`.
	// Each sound creates a pool of source voices for overlap support
	// (kSfxPoolSize). Failure logs via OutputDebugString and the slot
	// stays empty -- subsequent Play(name) is a no-op.
	void LoadSound(const std::string& name, const std::filesystem::path& path);

	// Triggers playback of the named sound. Picks the next voice in the
	// pool round-robin; concurrent triggers play simultaneously up to the
	// pool size before older instances get cycled out.
	void Play(const std::string& name);

	// Loads a 16-bit PCM WAV intended for looping background music. A
	// single dedicated source voice is created with XAUDIO2_LOOP_INFINITE.
	// LoadBgm replaces any previously registered BGM under the same name.
	void LoadBgm(const std::string& name, const std::filesystem::path& path);

	// Starts looping playback of the named BGM. If a different BGM is
	// already playing, stops it first. Calling with the same name as the
	// currently playing BGM is a no-op.
	void PlayBgm(const std::string& name);

	// Stops the currently playing BGM, if any.
	void StopBgm();

private:
	struct Sound;
	struct Bgm;
	void DestroySoundVoices();
	void DestroyBgmVoices();

	IXAudio2*                            xaudio_      = nullptr; // raw, ::Release in dtor
	IXAudio2MasteringVoice*              masterVoice_ = nullptr;
	std::unordered_map<std::string, Sound*> sounds_;
	std::unordered_map<std::string, Bgm*>   bgms_;
	std::string                          currentBgm_;
};
