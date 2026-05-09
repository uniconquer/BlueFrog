#include "AudioEngine.h"

#include "../../Core/BFWin.h"
#include <xaudio2.h>

#include <cstdint>
#include <cstring>
#include <fstream>
#include <string>
#include <vector>

#pragma comment(lib, "xaudio2.lib")

struct AudioEngine::Sound
{
	WAVEFORMATEX           format = {};
	std::vector<BYTE>      data;
	IXAudio2SourceVoice*   voice = nullptr; // owned; destroyed in AudioEngine dtor
};

namespace
{
	// Read 4 ASCII bytes as a uint32 in little-endian (RIFF tag).
	bool ReadFourCC(std::ifstream& f, char out[4])
	{
		f.read(out, 4);
		return f.good();
	}

	template <typename T>
	bool ReadLE(std::ifstream& f, T& out)
	{
		f.read(reinterpret_cast<char*>(&out), sizeof(T));
		return f.good();
	}

	// Parse a minimal 16-bit PCM WAV file: RIFF header, fmt subchunk,
	// data subchunk. Other chunks (LIST, etc.) are skipped. On success,
	// `outFormat` and `outData` are populated and the function returns
	// true; otherwise both are left in an unspecified state.
	bool ParseWav(const std::filesystem::path& path, WAVEFORMATEX& outFormat, std::vector<BYTE>& outData)
	{
		std::ifstream f(path, std::ios::binary);
		if (!f.is_open()) return false;

		char tag[4];
		if (!ReadFourCC(f, tag)) return false;
		if (std::memcmp(tag, "RIFF", 4) != 0) return false;
		std::uint32_t riffSize = 0;
		if (!ReadLE(f, riffSize)) return false;
		if (!ReadFourCC(f, tag)) return false;
		if (std::memcmp(tag, "WAVE", 4) != 0) return false;

		bool fmtSeen = false;
		bool dataSeen = false;
		while (f.good() && !(fmtSeen && dataSeen))
		{
			if (!ReadFourCC(f, tag)) break;
			std::uint32_t chunkSize = 0;
			if (!ReadLE(f, chunkSize)) break;

			if (std::memcmp(tag, "fmt ", 4) == 0)
			{
				// Read minimum 16 bytes of WAVEFORMATEX. chunkSize may be
				// 16 (PCM, no cbSize) or 18+ (extensible). We read what
				// the chunk size says, but only commit the first 16 to
				// our local WAVEFORMATEX (we don't use cbSize / extension).
				const std::uint32_t copyBytes = (chunkSize < sizeof(WAVEFORMATEX)) ? chunkSize : sizeof(WAVEFORMATEX);
				std::memset(&outFormat, 0, sizeof(WAVEFORMATEX));
				f.read(reinterpret_cast<char*>(&outFormat), copyBytes);
				if (chunkSize > copyBytes)
				{
					f.seekg(chunkSize - copyBytes, std::ios::cur);
				}
				// Ensure even-aligned chunk (RIFF spec pads to even bytes).
				if (chunkSize & 1u) f.seekg(1, std::ios::cur);
				fmtSeen = true;
			}
			else if (std::memcmp(tag, "data", 4) == 0)
			{
				outData.resize(chunkSize);
				f.read(reinterpret_cast<char*>(outData.data()), chunkSize);
				if (chunkSize & 1u) f.seekg(1, std::ios::cur);
				dataSeen = true;
			}
			else
			{
				// Unknown chunk; skip its payload.
				f.seekg(chunkSize, std::ios::cur);
				if (chunkSize & 1u) f.seekg(1, std::ios::cur);
			}
		}

		if (!fmtSeen || !dataSeen) return false;
		// Only PCM 16-bit is in scope for v1.
		if (outFormat.wFormatTag != WAVE_FORMAT_PCM) return false;
		if (outFormat.wBitsPerSample != 16) return false;
		return true;
	}

	void Log(const char* msg)
	{
		::OutputDebugStringA(msg);
		::OutputDebugStringA("\n");
	}
}

AudioEngine::AudioEngine()
{
	// COM is already initialized by ImageLoader on first surface load; if
	// AudioEngine is the first COM consumer, init here ourselves. Either
	// way the call is idempotent for the same threading model.
	const HRESULT coHr = ::CoInitializeEx(nullptr, COINIT_MULTITHREADED);
	if (FAILED(coHr) && coHr != RPC_E_CHANGED_MODE)
	{
		Log("[AudioEngine] CoInitializeEx failed; audio disabled");
		return;
	}

	if (FAILED(::XAudio2Create(&xaudio_, 0u, XAUDIO2_DEFAULT_PROCESSOR)))
	{
		Log("[AudioEngine] XAudio2Create failed; audio disabled");
		xaudio_ = nullptr;
		return;
	}

	if (FAILED(xaudio_->CreateMasteringVoice(&masterVoice_)))
	{
		Log("[AudioEngine] CreateMasteringVoice failed; audio disabled");
		xaudio_->Release();
		xaudio_ = nullptr;
		return;
	}
}

AudioEngine::~AudioEngine()
{
	DestroySoundVoices();
	if (masterVoice_)
	{
		masterVoice_->DestroyVoice();
		masterVoice_ = nullptr;
	}
	if (xaudio_)
	{
		xaudio_->Release();
		xaudio_ = nullptr;
	}
}

void AudioEngine::DestroySoundVoices()
{
	for (auto& kv : sounds_)
	{
		if (kv.second && kv.second->voice)
		{
			kv.second->voice->DestroyVoice();
			kv.second->voice = nullptr;
		}
		delete kv.second;
	}
	sounds_.clear();
}

void AudioEngine::LoadSound(const std::string& name, const std::filesystem::path& path)
{
	if (!xaudio_)
	{
		Log("[AudioEngine] LoadSound called before init; skipping");
		return;
	}

	WAVEFORMATEX format = {};
	std::vector<BYTE> data;
	if (!ParseWav(path, format, data))
	{
		std::string err = "[AudioEngine] LoadSound failed: " + path.string();
		Log(err.c_str());
		return;
	}

	IXAudio2SourceVoice* voice = nullptr;
	if (FAILED(xaudio_->CreateSourceVoice(&voice, &format)))
	{
		Log("[AudioEngine] CreateSourceVoice failed");
		return;
	}

	auto* slot = new Sound();
	slot->format = format;
	slot->data   = std::move(data);
	slot->voice  = voice;

	auto it = sounds_.find(name);
	if (it != sounds_.end() && it->second)
	{
		if (it->second->voice) it->second->voice->DestroyVoice();
		delete it->second;
	}
	sounds_[name] = slot;
}

void AudioEngine::Play(const std::string& name)
{
	auto it = sounds_.find(name);
	if (it == sounds_.end() || it->second == nullptr || it->second->voice == nullptr) return;

	Sound& s = *it->second;
	// Restart-on-replay semantics: stop, flush queued buffers, submit fresh.
	s.voice->Stop(0u);
	s.voice->FlushSourceBuffers();

	XAUDIO2_BUFFER buf = {};
	buf.AudioBytes = static_cast<UINT32>(s.data.size());
	buf.pAudioData = s.data.data();
	buf.Flags = XAUDIO2_END_OF_STREAM;

	if (FAILED(s.voice->SubmitSourceBuffer(&buf)))
	{
		Log("[AudioEngine] SubmitSourceBuffer failed");
		return;
	}
	s.voice->Start(0u);
}
