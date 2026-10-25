/*
 Copyright (C) 2024 BeamMP Ltd., BeamMP team and contributors.
 Licensed under AGPL-3.0 (or later), see <https://www.gnu.org/licenses/>.
 SPDX-License-Identifier: AGPL-3.0-or-later
*/

#include "Audio/VoiceChat.h"
#include "Logger.h"

#include <portaudio.h>
#include <opus/opus.h>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstring>
#include <sstream>

namespace {
    // Explicit little-endian float read — portable to any architecture.
    // Wire format: little-endian IEEE 754 floats (matches server BuildPacket).
    inline float readLEFloat(const uint8_t* p) {
        uint32_t bits = static_cast<uint32_t>(p[0])
                      | (static_cast<uint32_t>(p[1]) <<  8)
                      | (static_cast<uint32_t>(p[2]) << 16)
                      | (static_cast<uint32_t>(p[3]) << 24);
        float f; std::memcpy(&f, &bits, sizeof(float));
        return f;
    }
} // namespace

VoiceChat& VoiceChat::Instance() {
    static VoiceChat instance;
    return instance;
}

VoiceChat::~VoiceChat() {
    Shutdown();
}

// ── stream helpers ──────────────────────────────────────

void VoiceChat::OpenCaptureStream(int deviceId) {
    if (mCaptureStream) {
        Pa_StopStream(mCaptureStream);
        Pa_CloseStream(mCaptureStream);
        mCaptureStream = nullptr;
    }

    PaDeviceIndex dev = (deviceId < 0) ? Pa_GetDefaultInputDevice() : static_cast<PaDeviceIndex>(deviceId);
    if (dev == paNoDevice) {
        warn("VoiceChat: No input device available.");
        return;
    }
    const PaDeviceInfo* devInfo = Pa_GetDeviceInfo(dev);
    if (!devInfo || devInfo->maxInputChannels < 1) {
        warn("VoiceChat: Selected input device has no input channels.");
        return;
    }

    PaStreamParameters inputParams {};
    inputParams.device = dev;
    inputParams.channelCount = CHANNELS;
    inputParams.sampleFormat = paInt16;
    inputParams.suggestedLatency = devInfo->defaultLowInputLatency;

    PaError err = Pa_OpenStream(&mCaptureStream, &inputParams, nullptr,
        SAMPLE_RATE, FRAME_SIZE, paClipOff, CaptureCallback, this);
    if (err != paNoError) {
        error(std::string("VoiceChat: Failed to open capture stream: ") + Pa_GetErrorText(err));
        mCaptureStream = nullptr;
    } else {
        // Start stream immediately so mic level monitoring is always active
        PaError startErr = Pa_StartStream(mCaptureStream);
        if (startErr != paNoError) {
            error(std::string("VoiceChat: Failed to start capture stream: ") + Pa_GetErrorText(startErr));
            Pa_CloseStream(mCaptureStream);
            mCaptureStream = nullptr;
        } else {
            mCaptureDeviceId = dev;
            info(std::string("VoiceChat: Capture device: ") + devInfo->name);
        }
    }
}

void VoiceChat::OpenPlaybackStream(int deviceId) {
    if (mPlaybackStream) {
        Pa_StopStream(mPlaybackStream);
        Pa_CloseStream(mPlaybackStream);
        mPlaybackStream = nullptr;
    }

    PaDeviceIndex dev = (deviceId < 0) ? Pa_GetDefaultOutputDevice() : static_cast<PaDeviceIndex>(deviceId);
    if (dev == paNoDevice) {
        warn("VoiceChat: No output device available.");
        return;
    }
    const PaDeviceInfo* devInfo = Pa_GetDeviceInfo(dev);
    if (!devInfo || devInfo->maxOutputChannels < 1) {
        warn("VoiceChat: Selected output device has no output channels.");
        return;
    }

    int chCount = (devInfo->maxOutputChannels >= PLAYBACK_CHANNELS) ? PLAYBACK_CHANNELS : 1;

    PaStreamParameters outputParams {};
    outputParams.device = dev;
    outputParams.channelCount = chCount;
    outputParams.sampleFormat = paFloat32;
    outputParams.suggestedLatency = devInfo->defaultLowOutputLatency;

    PaError err = Pa_OpenStream(&mPlaybackStream, nullptr, &outputParams,
        SAMPLE_RATE, PLAYBACK_FRAME_SIZE, paClipOff, PlaybackCallback, this);
    if (err != paNoError && chCount > 1) {
        // Fallback to mono
        warn("VoiceChat: Stereo playback failed, falling back to mono.");
        outputParams.channelCount = 1;
        err = Pa_OpenStream(&mPlaybackStream, nullptr, &outputParams,
            SAMPLE_RATE, PLAYBACK_FRAME_SIZE, paClipOff, PlaybackCallback, this);
    }
    if (err != paNoError) {
        error(std::string("VoiceChat: Failed to open playback stream: ") + Pa_GetErrorText(err));
        mPlaybackStream = nullptr;
    } else {
        mPlaybackChannels = outputParams.channelCount;
        Pa_StartStream(mPlaybackStream);
        mPlaybackDeviceId = dev;
        info(std::string("VoiceChat: Playback device: ") + devInfo->name
             + " (" + std::to_string(mPlaybackChannels) + "ch)");
    }
}

// ── init / shutdown ─────────────────────────────────────

void VoiceChat::Init() {
    if (mInitialized.load()) return;

    PaError err = Pa_Initialize();
    if (err != paNoError) {
        error(std::string("VoiceChat: PortAudio init failed: ") + Pa_GetErrorText(err));
        return;
    }

    int opusErr = 0;
    mEncoder = opus_encoder_create(SAMPLE_RATE, CHANNELS, OPUS_APPLICATION_VOIP, &opusErr);
    if (opusErr != OPUS_OK || !mEncoder) {
        error(std::string("VoiceChat: Opus encoder creation failed: ") + opus_strerror(opusErr));
        Pa_Terminate();
        return;
    }
    opus_encoder_ctl(mEncoder, OPUS_SET_BITRATE(OPUS_BITRATE));
    opus_encoder_ctl(mEncoder, OPUS_SET_SIGNAL(OPUS_SIGNAL_VOICE));

    OpenCaptureStream(-1);
    OpenPlaybackStream(-1);

    mInitialized.store(true);

    // Start background thread that sends mic level to the game
    // (must NOT be done inside the real-time PortAudio callback)
    mLevelThreadRunning.store(true);
    mLevelThread = std::thread(&VoiceChat::MicLevelSenderLoop, this);

    info("VoiceChat: Initialized successfully.");
}

void VoiceChat::Shutdown() {
    if (!mInitialized.load()) return;
    mInitialized.store(false);
    mRecording.store(false);
    mMuted.store(false);

    // Stop mic level sender thread
    mLevelThreadRunning.store(false);
    if (mLevelThread.joinable()) mLevelThread.join();

    if (mCaptureStream) {
        Pa_StopStream(mCaptureStream);
        Pa_CloseStream(mCaptureStream);
        mCaptureStream = nullptr;
    }
    if (mPlaybackStream) {
        Pa_StopStream(mPlaybackStream);
        Pa_CloseStream(mPlaybackStream);
        mPlaybackStream = nullptr;
    }
    if (mEncoder) {
        opus_encoder_destroy(mEncoder);
        mEncoder = nullptr;
    }
    {
        std::lock_guard lock(mPlaybackMutex);
        for (auto& [id, cv] : mClients) {
            if (cv.decoder) opus_decoder_destroy(cv.decoder);
        }
        mClients.clear();
        mLastVoiceNotified.clear();
    }
    {
        std::lock_guard lock(mCallbackMutex);
        mSendCallback = nullptr;
        mGameSendCallback = nullptr;
    }

    Pa_Terminate();
    info("VoiceChat: Shutdown.");
}

// ── callbacks / setters ─────────────────────────────────

void VoiceChat::SetSendCallback(SendCallback cb) {
    std::lock_guard lock(mCallbackMutex);
    mSendCallback = std::move(cb);
}

void VoiceChat::SetGameSendCallback(GameSendCallback cb) {
    std::lock_guard lock(mCallbackMutex);
    mGameSendCallback = std::move(cb);
}

void VoiceChat::SetMuted(bool muted) {
    mMuted.store(muted);
    debug(std::string("VoiceChat: Muted = ") + (muted ? "true" : "false"));
}

void VoiceChat::SetVolume(int vol) {
    mVolume.store(std::max(0, std::min(100, vol)));
    debug("VoiceChat: Volume = " + std::to_string(mVolume.load()));
}

// ── device management ───────────────────────────────────

void VoiceChat::SetInputDevice(int deviceId) {
    if (!mInitialized.load()) return;
    OpenCaptureStream(deviceId);
}

void VoiceChat::SetOutputDevice(int deviceId) {
    if (!mInitialized.load()) return;
    OpenPlaybackStream(deviceId);
}

std::string VoiceChat::EnumerateDevicesJson() {
    if (!mInitialized.load()) return "{}";
    int numDevices = Pa_GetDeviceCount();
    if (numDevices < 0) return "{}";

    // Prefer platform-native API to avoid duplicate devices across host APIs
#ifdef _WIN32
    PaHostApiIndex preferredApi = Pa_HostApiTypeIdToHostApiIndex(paWASAPI);
#elif defined(__linux__)
    PaHostApiIndex preferredApi = Pa_HostApiTypeIdToHostApiIndex(paPulseAudio);
    if (preferredApi < 0) preferredApi = Pa_HostApiTypeIdToHostApiIndex(paALSA);
#else
    PaHostApiIndex preferredApi = -1;
#endif
    if (preferredApi < 0) preferredApi = Pa_GetDefaultHostApi();

    std::ostringstream ss;
    ss << "{\"input\":[";
    bool firstIn = true;
    for (int i = 0; i < numDevices; ++i) {
        const PaDeviceInfo* info = Pa_GetDeviceInfo(i);
        if (!info || info->maxInputChannels < 1) continue;
        if (info->hostApi != preferredApi) continue;
        if (!firstIn) ss << ",";
        firstIn = false;
        std::string name(info->name);
        // Escape quotes in device name
        for (size_t p = 0; (p = name.find('"', p)) != std::string::npos; p += 2)
            name.replace(p, 1, "\\\"");
        ss << "{\"id\":" << i << ",\"name\":\"" << name << "\"}";
    }
    ss << "],\"output\":[";
    bool firstOut = true;
    for (int i = 0; i < numDevices; ++i) {
        const PaDeviceInfo* info = Pa_GetDeviceInfo(i);
        if (!info || info->maxOutputChannels < 1) continue;
        if (info->hostApi != preferredApi) continue;
        if (!firstOut) ss << ",";
        firstOut = false;
        std::string name(info->name);
        for (size_t p = 0; (p = name.find('"', p)) != std::string::npos; p += 2)
            name.replace(p, 1, "\\\"");
        ss << "{\"id\":" << i << ",\"name\":\"" << name << "\"}";
    }
    ss << "]}";
    return ss.str();
}

// ── recording ───────────────────────────────────────────

void VoiceChat::StartRecording() {
    if (!mInitialized.load() || !mCaptureStream) {
        warn("VoiceChat: StartRecording failed - not initialized or no capture stream");
        return;
    }
    if (mRecording.load()) return;
    {
        std::lock_guard lock(mCaptureMutex);
        mCaptureBuffer.clear();
    }
    mRecording.store(true);
    info("VoiceChat: Recording started (encoding active).");
}

void VoiceChat::StopRecording() {
    if (!mRecording.load()) return;
    mRecording.store(false);
    info("VoiceChat: Recording stopped (encoding paused).");
}

void VoiceChat::UpdateListenerPosition(float x, float y, float z) {
    std::lock_guard lock(mListenerMutex);
    mListenerPos[0] = x;
    mListenerPos[1] = y;
    mListenerPos[2] = z;
}

void VoiceChat::UpdateListenerOrientation(float fx, float fy, float fz) {
    // Normalize the forward vector
    float len = std::sqrt(fx*fx + fy*fy + fz*fz);
    if (len < 1e-6f) return;
    std::lock_guard lock(mListenerMutex);
    mListenerFwd[0] = fx / len;
    mListenerFwd[1] = fy / len;
    mListenerFwd[2] = fz / len;
}

// ── capture callback ────────────────────────────────────

int VoiceChat::CaptureCallback(const void* input, void* /*output*/,
    unsigned long frameCount, const PaStreamCallbackTimeInfo* /*timeInfo*/,
    unsigned long /*statusFlags*/, void* userData) {
    auto* self = static_cast<VoiceChat*>(userData);
    if (!input) return paContinue;

    const auto* samples = static_cast<const int16_t*>(input);

    // Always calculate RMS for mic level indicator (atomic store only — no blocking I/O)
    float rms = 0.0f;
    for (unsigned long i = 0; i < frameCount; ++i) {
        float s = samples[i] / 32768.0f;
        rms += s * s;
    }
    rms = std::sqrt(rms / frameCount);
    // Apply same gain factor so the meter reflects the actual transmitted level
    float MIC_GAIN = (self->mMicGainPct.load() / 100.0f) * 4.0f; // 100=4x default, 500=20x, 800=32x max
    rms = std::min(1.0f, rms * MIC_GAIN);
    self->mMicLevel.store(rms);

    // Only encode and transmit when actively recording and not muted
    if (!self->mRecording.load() || self->mMuted.load()) return paContinue;

    self->EncodeCapturedAudio(samples, frameCount);
    return paContinue;
}

void VoiceChat::SetMusicVolume(int vol) {
    mMusicVolume.store(std::max(0, std::min(100, vol)));
    debug("VoiceChat: MusicVolume = " + std::to_string(mMusicVolume.load()));
}

void VoiceChat::SetMicGain(int pct) {
    mMicGainPct.store(std::max(0, std::min(800, pct)));
    debug("VoiceChat: MicGainPct = " + std::to_string(mMicGainPct.load()));
}

void VoiceChat::MicLevelSenderLoop() {
    info("VoiceChat: Mic level sender thread started.");
    int lastLevel = -1;
    while (mLevelThreadRunning.load()) {
        GameSendCallback cbCopy;
        {
            std::lock_guard lock(mCallbackMutex);
            cbCopy = mGameSendCallback;
        }
        if (mCaptureStream && cbCopy) {
            float rms = mMicLevel.load();
            int levelPercent = std::min(100, static_cast<int>(rms * 100.0f));
            if (levelPercent != lastLevel) {
                lastLevel = levelPercent;
                cbCopy("Fl" + std::to_string(levelPercent));
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    info("VoiceChat: Mic level sender thread stopped.");
}

void VoiceChat::EncodeCapturedAudio(const int16_t* samples, size_t count) {
    if (!mEncoder) return;

    SendCallback cbCopy;
    {
        std::lock_guard lock(mCallbackMutex);
        cbCopy = mSendCallback;
    }

    std::lock_guard lock(mCaptureMutex);
    mCaptureBuffer.insert(mCaptureBuffer.end(), samples, samples + count);

    // Temp contiguous buffer for opus_encode (deque is not contiguous)
    std::vector<int16_t> frameBuffer(FRAME_SIZE);

    while (mCaptureBuffer.size() >= static_cast<size_t>(FRAME_SIZE)) {
        std::copy(mCaptureBuffer.begin(), mCaptureBuffer.begin() + FRAME_SIZE, frameBuffer.begin());

        // Software mic gain: dynamic, controlled via SetMicGain (Fg command).
        // Default mMicGainPct=100 -> 4.0x (~+12dB). Range 0-200%.
        float MIC_GAIN = (mMicGainPct.load() / 100.0f) * 4.0f; // 100=4x default, 500=20x, 800=32x max
        for (auto& s : frameBuffer) {
            float boosted = s * MIC_GAIN;
            s = static_cast<int16_t>(std::max(-32768.0f, std::min(32767.0f, boosted)));
        }

        unsigned char opusOut[MAX_OPUS_PACKET];
        int encoded = opus_encode(mEncoder, frameBuffer.data(), FRAME_SIZE, opusOut, MAX_OPUS_PACKET);

        if (encoded > 0 && cbCopy) {
            std::string packet(1 + encoded, '\0');
            packet[0] = 'F';
            std::memcpy(&packet[1], opusOut, encoded);
            cbCopy(packet, false);
        }

        mCaptureBuffer.erase(mCaptureBuffer.begin(), mCaptureBuffer.begin() + FRAME_SIZE);
    }
}

// ── incoming voice ──────────────────────────────────────

void VoiceChat::ProcessIncomingVoice(const char* data, size_t len) {
    if (len < VOICE_HEADER_SIZE + 1) return;

    // Packet v2: F + version(1) + flags(1) + source_id(2) + pos(3x4B) + maxDist(4B) + gain(4B) + opus
    const auto* p = reinterpret_cast<const uint8_t*>(data);
    uint8_t version = p[1];
    if (version != VOICE_PROTOCOL_VERSION) return;

    uint8_t flags = p[2];
    // Explicit LE reads — matches server BuildPacket wire format.
    uint16_t sourceId = static_cast<uint16_t>(p[3]) | (static_cast<uint16_t>(p[4]) << 8);
    float pos[3]   = { readLEFloat(p + 5), readLEFloat(p + 9), readLEFloat(p + 13) };
    float maxDistance   = readLEFloat(p + 17);
    float broadcastGain = readLEFloat(p + 21);
    if (broadcastGain < 0.0f) broadcastGain = 0.0f;
    if (broadcastGain > 1.0f) broadcastGain = 1.0f; // server sends [0.0, 1.0]

    const unsigned char* opusData = reinterpret_cast<const unsigned char*>(data + VOICE_HEADER_SIZE);
    int opusLen = static_cast<int>(len - VOICE_HEADER_SIZE);

    // Snapshot callback before taking the heavy lock
    GameSendCallback cbCopy;
    {
        std::lock_guard lockCb(mCallbackMutex);
        cbCopy = mGameSendCallback;
    }

    std::lock_guard lock(mPlaybackMutex);

    // Throttled "speaking" notification — mLastVoiceNotified is protected by mPlaybackMutex.
    if (cbCopy) {
        auto now = std::chrono::steady_clock::now();
        auto& last = mLastVoiceNotified[sourceId];
        if (std::chrono::duration_cast<std::chrono::milliseconds>(now - last).count() >= 300) {
            last = now;
            bool isInjected = (flags & VOICE_FLAG_INJECTED) != 0;
            // Fk<sourceId>:<injected> — speaking notification
            cbCopy("Fk" + std::to_string(sourceId) + ":" + (isInjected ? "1" : "0"));
        }
    }

    auto& client = mClients[sourceId];
    if (!client.decoder) {
        int err = 0;
        client.decoder = opus_decoder_create(SAMPLE_RATE, CHANNELS, &err);
        if (err != OPUS_OK || !client.decoder) {
            error("VoiceChat: Failed to create Opus decoder for source " + std::to_string(sourceId));
            mClients.erase(sourceId);
            return;
        }
    }

    client.position[0] = pos[0];
    client.position[1] = pos[1];
    client.position[2] = pos[2];
    client.maxDistance = maxDistance;
    client.broadcastGain = broadcastGain;
    client.flags = flags;
    client.lastReceived = std::chrono::steady_clock::now();

    // Decode raw — attenuation is applied in MixAndPlay using current listener pos
    std::vector<float> decoded(FRAME_SIZE);
    int samples = opus_decode_float(client.decoder, opusData, opusLen, decoded.data(), FRAME_SIZE, 0);
    if (samples > 0) {
        decoded.resize(samples);

        // Compact consumed samples back to the front (O(n) but in the network
        // thread, never in the audio callback — see sampleReadPos in header).
        if (client.sampleReadPos > 0) {
            client.sampleQueue.erase(client.sampleQueue.begin(),
                                     client.sampleQueue.begin() + client.sampleReadPos);
            client.sampleReadPos = 0;
        }

        // Cap unread queue to ~500ms to prevent unbounded growth.
        constexpr size_t MAX_QUEUE = PLAYBACK_FRAME_SIZE * 25; // 25x20ms = 500ms
        if (client.sampleQueue.size() + decoded.size() > MAX_QUEUE) {
            size_t drop = (client.sampleQueue.size() + decoded.size()) - MAX_QUEUE;
            client.sampleQueue.erase(client.sampleQueue.begin(),
                                     client.sampleQueue.begin() + std::min(drop, client.sampleQueue.size()));
        }
        client.sampleQueue.insert(client.sampleQueue.end(), decoded.begin(), decoded.end());

        if (flags & VOICE_FLAG_INJECTED) {
            // Injected audio: server controls pacing, start immediately
            client.buffering = false;
        } else if (client.buffering
                && client.sampleQueue.size() >= static_cast<size_t>(PLAYBACK_FRAME_SIZE * JITTER_BUFFER_FRAMES)) {
            // Mic audio: wait for jitter buffer to fill
            client.buffering = false;
        }
    }
}

// ── playback ────────────────────────────────────────────

int VoiceChat::PlaybackCallback(const void* /*input*/, void* output,
    unsigned long frameCount, const PaStreamCallbackTimeInfo* /*timeInfo*/,
    unsigned long /*statusFlags*/, void* userData) {
    auto* self = static_cast<VoiceChat*>(userData);
    auto* out = static_cast<float*>(output);
    std::memset(out, 0, frameCount * self->mPlaybackChannels * sizeof(float));
    self->MixAndPlay(out, frameCount);
    return paContinue;
}

void VoiceChat::MixAndPlay(float* output, unsigned long frameCount) {
    // Regular lock: ProcessIncomingVoice holds this for only ~microseconds (one Opus
    // decode), so blocking here is far cheaper than the glitch from try_lock dropping
    // an entire 20ms frame every time a network packet arrives simultaneously.
    std::unique_lock lock(mPlaybackMutex);

    // Snapshot listener position and orientation under its mutex
    float listenerPos[3], listenerFwd[3];
    {
        std::unique_lock lpos(mListenerMutex, std::try_to_lock);
        if (lpos.owns_lock()) {
            listenerPos[0] = mListenerPos[0];
            listenerPos[1] = mListenerPos[1];
            listenerPos[2] = mListenerPos[2];
            listenerFwd[0] = mListenerFwd[0];
            listenerFwd[1] = mListenerFwd[1];
            listenerFwd[2] = mListenerFwd[2];
        } else {
            listenerPos[0] = listenerPos[1] = listenerPos[2] = 0.0f;
            listenerFwd[0] = 0.0f; listenerFwd[1] = 1.0f; listenerFwd[2] = 0.0f;
        }
    }

    // Voice volume: slider 0-100 -> gain 0.0-3.0 (+9.5dB headroom for quiet mics)
    float volScale = (mVolume.load() / 100.0f) * 3.0f;
    // Music volume: separate slider 0-100 -> gain 0.0-3.0 (injected sources only)
    float musicVolScale = (mMusicVolume.load() / 100.0f) * 3.0f;
    auto now = std::chrono::steady_clock::now();
    std::vector<uint16_t> expired;

    for (auto& [id, client] : mClients) {
        if (std::chrono::duration_cast<std::chrono::seconds>(now - client.lastReceived).count() > 2) {
            expired.push_back(id);
            continue;
        }

        if (client.buffering) continue;

        // ── Compute target gain from current listener position ──────────────
        float targetGain = 1.0f;
        if (client.flags & VOICE_FLAG_PROXIMITY) {
            // maxDistance==0 means the server set proxDist=0 = unlimited range.
            // In that case skip falloff entirely so players always hear each other
            // regardless of whatever position the server embedded in the packet.
            if (client.maxDistance > 0.0f) {
                float dx = client.position[0] - listenerPos[0];
                float dy = client.position[1] - listenerPos[1];
                float dz = client.position[2] - listenerPos[2];
                float dist = std::sqrt(dx * dx + dy * dy + dz * dz);

                float maxDist = client.maxDistance;
                if (dist >= maxDist) {
                    targetGain = 0.0f;
                } else {
                    float innerRadius = maxDist * 0.15f;
                    if (dist <= innerRadius) {
                        targetGain = 1.0f;
                    } else {
                        float t = (dist - innerRadius) / (maxDist - innerRadius);
                        targetGain = 1.0f - std::pow(t, 0.5f);
                        targetGain = std::max(0.0f, targetGain);
                    }
                }
            }
            // else: maxDistance==0 -> unlimited, keep targetGain=1.0f
        }
        // ── Compute target stereo pan from source angle relative to listener forward ──
        float dx = client.position[0] - listenerPos[0];
        float dy = client.position[1] - listenerPos[1];
        // Right vector perpendicular to forward in XY plane (BeamNG coords)
        float rightX =  listenerFwd[1];
        float rightY = -listenerFwd[0];
        float srcLen = std::sqrt(dx*dx + dy*dy);
        float targetPan = 0.0f; // -1=full left, 0=center, +1=full right
        if (srcLen > 0.5f) {
            targetPan = (dx * rightX + dy * rightY) / srcLen;
            targetPan = std::max(-1.0f, std::min(1.0f, targetPan));
        }

        // ── Per-sample smoothing: slow for gain (avoids clicks), fast for pan (responsive) ──
        // gainAlpha ~0.05 -> ~20 samples to settle (smooth volume transitions)
        // panAlpha  ~0.30 -> ~3  samples to settle (near-instant pan on camera rotation)
        const float gainAlpha = 0.05f;
        const float panAlpha  = 0.30f;
        // Use music volume for injected sources, voice volume for mic sources
        float sourceVolScale = (client.flags & VOICE_FLAG_INJECTED) ? musicVolScale : volScale;
        // Apply broadcast gain (sender's car stereo volume) for injected sources
        float bGain = (client.flags & VOICE_FLAG_INJECTED) ? client.broadcastGain : 1.0f;
        targetGain *= sourceVolScale * bGain;

        // O(1) read: advance the read head, compaction is deferred to ProcessIncomingVoice.
        const size_t available = client.sampleQueue.size() - client.sampleReadPos;
        size_t toMix = std::min(static_cast<size_t>(frameCount), available);
        if (mPlaybackChannels >= 2) {
            for (size_t i = 0; i < toMix; ++i) {
                client.smoothedGain += gainAlpha * (targetGain - client.smoothedGain);
                client.smoothedPan  += panAlpha  * (targetPan  - client.smoothedPan);
                // Equal-power panning from smoothed pan value
                float angle = (client.smoothedPan + 1.0f) * 0.7854f; // (pan+1)*pi/4
                float gL = std::cos(angle) * client.smoothedGain;
                float gR = std::sin(angle) * client.smoothedGain;
                float s = client.sampleQueue[client.sampleReadPos + i];
                output[i * 2 + 0] += s * gL;
                output[i * 2 + 1] += s * gR;
            }
        } else {
            for (size_t i = 0; i < toMix; ++i) {
                client.smoothedGain += gainAlpha * (targetGain - client.smoothedGain);
                output[i] += client.sampleQueue[client.sampleReadPos + i] * client.smoothedGain;
            }
        }
        client.sampleReadPos += toMix; // O(1) — no erase in audio callback

        // Do not re-enable buffering mid-stream — only on fresh decoder creation
    }

    for (auto id : expired) {
        if (mClients[id].decoder) opus_decoder_destroy(mClients[id].decoder);
        mClients.erase(id);
    }

    // Clip output (stereo or mono)
    for (unsigned long i = 0; i < frameCount * static_cast<unsigned long>(mPlaybackChannels); ++i) {
        output[i] = std::max(-1.0f, std::min(1.0f, output[i]));
    }
}
