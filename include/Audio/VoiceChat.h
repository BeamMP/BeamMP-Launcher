/*
 Copyright (C) 2024 BeamMP Ltd., BeamMP team and contributors.
 Licensed under AGPL-3.0 (or later), see <https://www.gnu.org/licenses/>.
 SPDX-License-Identifier: AGPL-3.0-or-later
*/

#pragma once

#include <atomic>
#include <cstdint>
#include <functional>
#include <mutex>
#include <string>
#include <unordered_map>
#include <deque>
#include <vector>
#include <chrono>
#include <thread>

struct PaStreamParameters;
struct PaStreamCallbackTimeInfo;
typedef void PaStream;
struct OpusEncoder;
struct OpusDecoder;

class VoiceChat {
public:
    using SendCallback = std::function<void(const std::string&, bool)>;
    using GameSendCallback = std::function<void(const std::string&)>;

    static VoiceChat& Instance();

    void Init();
    void Shutdown();
    void SetSendCallback(SendCallback cb);
    void SetGameSendCallback(GameSendCallback cb);

    void StartRecording();
    void StopRecording();
    void SetMuted(bool muted);
    void SetVolume(int vol);
    void SetMusicVolume(int vol);  // 0-100, applies only to injected (music) audio
    void SetMicGain(int pct);      // 0-200, 100=default (4x). Scales software mic gain.
    void UpdateListenerPosition(float x, float y, float z);
    void UpdateListenerOrientation(float fx, float fy, float fz);

    void SetInputDevice(int deviceId);
    void SetOutputDevice(int deviceId);
    std::string EnumerateDevicesJson();

    void ProcessIncomingVoice(const char* data, size_t len);

    bool IsInitialized() const { return mInitialized.load(); }
    bool IsRecording() const { return mRecording.load(); }
    bool IsMuted() const { return mMuted.load(); }

    static constexpr int SAMPLE_RATE = 48000;
    static constexpr int CAPTURE_CHANNELS  = 1;  // mic: mono
    static constexpr int PLAYBACK_CHANNELS = 2;  // output: stereo
    static constexpr int CHANNELS = CAPTURE_CHANNELS; // legacy alias for encoder
    // 20ms is the standard Opus frame size for low-latency voice chat and matches
    // the music-server injection cadence.  Both capture and playback use the same
    // size so the encoder/decoder pair is symmetric.
    // If PortAudio callback scheduling causes glitches at 20ms, raise to 40ms.
    static constexpr int FRAME_DURATION_MS = 20;           // mic capture: 20ms frames
    static constexpr int FRAME_SIZE = SAMPLE_RATE * FRAME_DURATION_MS / 1000; // 960 — capture/encoder
    static constexpr int PLAYBACK_FRAME_DURATION_MS = 20;  // playback callback: 20ms
    static constexpr int PLAYBACK_FRAME_SIZE = SAMPLE_RATE * PLAYBACK_FRAME_DURATION_MS / 1000; // 960
    static constexpr int MAX_OPUS_PACKET = 512;
    static constexpr int OPUS_BITRATE = 24000; // bps — balance of quality and bandwidth
    static constexpr int JITTER_BUFFER_FRAMES = 3; // buffer 3x20ms = 60ms before playback starts (absorbs network jitter)
    // Packet v2: 'F' + uint8(version) + uint8(flags) + uint16(source_id) + float[3](pos)
    static constexpr size_t VOICE_HEADER_SIZE = 1 + 1 + 1 + 2 + 12 + 4 + 4; // 25 bytes: F+ver+flags+id+pos+maxDist+gain
    static constexpr uint8_t VOICE_PROTOCOL_VERSION = 2;
    static constexpr uint8_t VOICE_FLAG_PROXIMITY = 0x01;
    static constexpr uint8_t VOICE_FLAG_INJECTED  = 0x02;

private:
    VoiceChat() = default;
    ~VoiceChat();
    VoiceChat(const VoiceChat&) = delete;
    VoiceChat& operator=(const VoiceChat&) = delete;

    static int CaptureCallback(const void* input, void* output,
        unsigned long frameCount, const PaStreamCallbackTimeInfo* timeInfo,
        unsigned long statusFlags, void* userData);

    void EncodeCapturedAudio(const int16_t* samples, size_t count);
    void MixAndPlay(float* output, unsigned long frameCount);

    static int PlaybackCallback(const void* input, void* output,
        unsigned long frameCount, const PaStreamCallbackTimeInfo* timeInfo,
        unsigned long statusFlags, void* userData);

    void OpenCaptureStream(int deviceId);
    void OpenPlaybackStream(int deviceId);
    void MicLevelSenderLoop();

    std::atomic<bool> mInitialized { false };
    std::atomic<bool> mRecording { false };
    std::atomic<bool> mMuted { false };
    std::atomic<int> mVolume { 80 };
    std::atomic<int> mMusicVolume { 100 };  // music (injected) volume 0-100
    std::atomic<int> mMicGainPct { 100 };   // mic gain: (pct/100)*4 = actual multiplier. 100=4x default, 800=32x max
    std::atomic<float> mMicLevel { 0.0f };
    std::atomic<bool> mLevelThreadRunning { false };
    std::thread mLevelThread;

    SendCallback mSendCallback;
    GameSendCallback mGameSendCallback;
    std::mutex mCallbackMutex;

    PaStream* mCaptureStream = nullptr;
    int mCaptureDeviceId = -1;
    OpusEncoder* mEncoder = nullptr;
    std::deque<int16_t> mCaptureBuffer;
    std::mutex mCaptureMutex;

    PaStream* mPlaybackStream = nullptr;
    int mPlaybackDeviceId = -1;
    int mPlaybackChannels = PLAYBACK_CHANNELS; // actual opened channel count (may fall back to 1)

    struct ClientVoice {
        OpusDecoder* decoder = nullptr;
        // Sample ring: written by ProcessIncomingVoice (network thread),
        // read by MixAndPlay (audio callback thread) — both hold mPlaybackMutex.
        // sampleReadPos is the read head; the vector is compacted back to zero in
        // ProcessIncomingVoice (network thread) so the audio callback never calls
        // erase(), keeping the hot path O(1).
        std::vector<float> sampleQueue;
        size_t sampleReadPos = 0;
        float position[3] = { 0.0f, 0.0f, 0.0f };
        float maxDistance = 0.0f;
        float broadcastGain = 1.0f; // gain set by sender (car stereo volume), 0.0-1.0
        float smoothedGain = 1.0f;  // interpolated gain to avoid discontinuities
        float smoothedPan  = 0.0f;  // interpolated pan (-1..+1) for smooth rotation
        uint8_t flags = 0;          // last received flags (proximity, injected)
        std::chrono::steady_clock::time_point lastReceived;
        bool buffering = true; // jitter buffer: true until enough frames accumulated
    };
    std::unordered_map<uint16_t, ClientVoice> mClients;
    // Throttle map for "speaking" notifications — protected by mPlaybackMutex.
    // Declared as a member (not static local) so it is cleared on Shutdown().
    std::unordered_map<uint16_t, std::chrono::steady_clock::time_point> mLastVoiceNotified;
    std::mutex mPlaybackMutex;

    float mListenerPos[3] = { 0.0f, 0.0f, 0.0f };
    float mListenerFwd[3] = { 0.0f, 1.0f, 0.0f }; // forward direction (normalized)
    std::mutex mListenerMutex;
};
