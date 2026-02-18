#pragma once

// =============================================================================
// tcxNDIReceiver.h - NDI video/audio receiver with source discovery
// =============================================================================

#include <TrussC.h>
#include <Processing.NDI.Lib.h>
#include <string>
#include <vector>
#include <cstring>

namespace tcx {

// NDI source information
struct NDISource {
    std::string name;
    std::string url;
};

class NDIReceiver {
public:
    NDIReceiver() = default;

    ~NDIReceiver() {
        disconnect();
        if (ndiFinder_) {
            NDIlib_find_destroy(ndiFinder_);
            ndiFinder_ = nullptr;
        }
    }

    // Non-copyable
    NDIReceiver(const NDIReceiver&) = delete;
    NDIReceiver& operator=(const NDIReceiver&) = delete;

    // =========================================================================
    // Source Discovery
    // =========================================================================

    std::vector<NDISource> findSources(uint32_t timeoutMs = 1000) {
        if (!NDIlib_initialize()) {
            tc::logError() << "[NDIReceiver] Failed to initialize NDI runtime";
            return {};
        }

        // Create finder if needed
        if (!ndiFinder_) {
            NDIlib_find_create_t findDesc;
            findDesc.show_local_sources = true;
            findDesc.p_groups = nullptr;
            findDesc.p_extra_ips = nullptr;
            ndiFinder_ = NDIlib_find_create_v2(&findDesc);
            if (!ndiFinder_) {
                tc::logError() << "[NDIReceiver] Failed to create NDI finder";
                return {};
            }
        }

        // Wait for sources
        NDIlib_find_wait_for_sources(ndiFinder_, timeoutMs);

        // Get current sources
        uint32_t numSources = 0;
        const NDIlib_source_t* sources = NDIlib_find_get_current_sources(ndiFinder_, &numSources);

        std::vector<NDISource> result;
        cachedSources_.clear();

        for (uint32_t i = 0; i < numSources; i++) {
            NDISource src;
            src.name = sources[i].p_ndi_name ? sources[i].p_ndi_name : "";
            src.url = sources[i].p_url_address ? sources[i].p_url_address : "";
            result.push_back(src);
            cachedSources_.push_back(sources[i]);
        }

        return result;
    }

    int getSourceCount() const { return (int)cachedSources_.size(); }

    // =========================================================================
    // Connection
    // =========================================================================

    bool connect(const NDISource& source) {
        disconnect();

        if (!NDIlib_initialize()) return false;

        // Find the NDI source by name
        NDIlib_source_t ndiSource;
        ndiSource.p_ndi_name = source.name.c_str();
        ndiSource.p_url_address = source.url.empty() ? nullptr : source.url.c_str();

        NDIlib_recv_create_v3_t recvDesc;
        recvDesc.source_to_connect_to = ndiSource;
        recvDesc.color_format = NDIlib_recv_color_format_RGBX_RGBA;
        recvDesc.bandwidth = lowBandwidth_
            ? NDIlib_recv_bandwidth_lowest : NDIlib_recv_bandwidth_highest;
        recvDesc.allow_video_fields = true;
        recvDesc.p_ndi_recv_name = nullptr;

        ndiRecv_ = NDIlib_recv_create_v3(&recvDesc);
        if (!ndiRecv_) {
            tc::logError() << "[NDIReceiver] Failed to create NDI receiver";
            return false;
        }

        sourceName_ = source.name;
        connected_ = true;

        tc::logNotice() << "[NDIReceiver] Connected to: " << sourceName_;
        return true;
    }

    bool connect(const std::string& sourceName) {
        NDISource src;
        src.name = sourceName;
        return connect(src);
    }

    bool connect(int sourceIndex) {
        if (sourceIndex < 0 || sourceIndex >= (int)cachedSources_.size()) return false;

        NDISource src;
        src.name = cachedSources_[sourceIndex].p_ndi_name ? cachedSources_[sourceIndex].p_ndi_name : "";
        src.url = cachedSources_[sourceIndex].p_url_address ? cachedSources_[sourceIndex].p_url_address : "";
        return connect(src);
    }

    void disconnect() {
        if (ndiRecv_) {
            NDIlib_recv_destroy(ndiRecv_);
            ndiRecv_ = nullptr;
        }
        connected_ = false;
        sourceName_.clear();
        width_ = 0;
        height_ = 0;
        fps_ = 0;
    }

    bool isConnected() const { return connected_; }

    // =========================================================================
    // Receive Video
    // =========================================================================

    // Receive into Pixels (allocates/resizes as needed)
    bool receive(tc::Pixels& pixels) {
        if (!connected_ || !ndiRecv_) return false;

        frameNew_ = false;

        NDIlib_video_frame_v2_t video;
        NDIlib_frame_type_e type = NDIlib_recv_capture_v2(ndiRecv_, &video, nullptr, nullptr, 0);

        if (type == NDIlib_frame_type_video) {
            width_ = video.xres;
            height_ = video.yres;
            if (video.frame_rate_D > 0) {
                fps_ = (float)video.frame_rate_N / (float)video.frame_rate_D;
            }

            // Ensure pixels buffer is correct size
            if (!pixels.isAllocated() || pixels.getWidth() != width_ || pixels.getHeight() != height_) {
                pixels.allocate(width_, height_, 4);
            }

            // Copy pixel data
            int srcStride = video.line_stride_in_bytes;
            int dstStride = width_ * 4;

            if (srcStride == dstStride) {
                std::memcpy(pixels.getData(), video.p_data, (size_t)height_ * dstStride);
            } else {
                // Handle stride mismatch
                for (int y = 0; y < height_; y++) {
                    std::memcpy(pixels.getData() + y * dstStride,
                                video.p_data + y * srcStride,
                                dstStride);
                }
            }

            NDIlib_recv_free_video_v2(ndiRecv_, &video);
            frameNew_ = true;
            return true;
        }

        return false;
    }

    // Receive into Texture (CPU readback → texture upload)
    bool receive(tc::Texture& texture) {
        if (!receive(tempPixels_)) return false;

        int w = tempPixels_.getWidth();
        int h = tempPixels_.getHeight();
        int ch = tempPixels_.getChannels();

        // Allocate or reallocate texture if size changed
        if (!texture.isAllocated() ||
            texture.getWidth() != w || texture.getHeight() != h) {
            texture.allocate(w, h, ch, tc::TextureUsage::Dynamic);
        }

        texture.loadData(tempPixels_);
        return true;
    }

    bool isFrameNew() const { return frameNew_; }

    // =========================================================================
    // Source Info
    // =========================================================================

    std::string getSourceName() const { return sourceName_; }
    int getWidth() const { return width_; }
    int getHeight() const { return height_; }
    float getFps() const { return fps_; }

    // =========================================================================
    // Settings
    // =========================================================================

    void setLowBandwidth(bool low) { lowBandwidth_ = low; }

    // =========================================================================
    // Audio
    // =========================================================================

    void setAudioEnabled(bool enabled) { audioEnabled_ = enabled; }
    bool isAudioEnabled() const { return audioEnabled_; }

    bool receiveAudio(float* samples, int& numSamples, int& numChannels) {
        if (!connected_ || !ndiRecv_ || !audioEnabled_) return false;

        NDIlib_audio_frame_v2_t audio;
        NDIlib_frame_type_e type = NDIlib_recv_capture_v2(ndiRecv_, nullptr, &audio, nullptr, 0);

        if (type == NDIlib_frame_type_audio) {
            numSamples = audio.no_samples;
            numChannels = audio.no_channels;
            audioSampleRate_ = audio.sample_rate;
            audioChannels_ = audio.no_channels;

            // Copy audio data (planar float)
            if (samples) {
                for (int ch = 0; ch < numChannels; ch++) {
                    const float* src = audio.p_data + ch * (audio.channel_stride_in_bytes / sizeof(float));
                    std::memcpy(samples + ch * numSamples, src, numSamples * sizeof(float));
                }
            }

            NDIlib_recv_free_audio_v2(ndiRecv_, &audio);
            return true;
        }

        return false;
    }

    bool hasAudio() const { return audioEnabled_ && connected_; }
    int getAudioSampleRate() const { return audioSampleRate_; }
    int getAudioChannels() const { return audioChannels_; }

private:
    NDIlib_find_instance_t ndiFinder_ = nullptr;
    NDIlib_recv_instance_t ndiRecv_ = nullptr;

    std::string sourceName_;
    int width_ = 0, height_ = 0;
    float fps_ = 0;
    bool connected_ = false;
    bool frameNew_ = false;
    bool lowBandwidth_ = false;
    bool audioEnabled_ = false;
    int audioSampleRate_ = 48000;
    int audioChannels_ = 2;

    // Cached NDI sources from last findSources()
    std::vector<NDIlib_source_t> cachedSources_;

    // Temp pixels for texture receive path
    tc::Pixels tempPixels_;
};

} // namespace tcx
