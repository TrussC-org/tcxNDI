#pragma once

// =============================================================================
// tcxNDISender.h - NDI video/audio sender
// =============================================================================

#include <TrussC.h>
#include <Processing.NDI.Lib.h>
#include <string>
#include <vector>
#include <cstring>
#include <cmath>

namespace tcx::ndi {

class NDISender {
public:
    NDISender() = default;

    ~NDISender() {
        close();
    }

    // Non-copyable
    NDISender(const NDISender&) = delete;
    NDISender& operator=(const NDISender&) = delete;

    // =========================================================================
    // Setup / Cleanup
    // =========================================================================

    bool setup(const std::string& name, int width, int height) {
        close();

        if (!NDIlib_initialize()) {
            tc::logError() << "[NDISender] Failed to initialize NDI runtime";
            return false;
        }

        name_ = name;
        width_ = width;
        height_ = height;

        NDIlib_send_create_t sendDesc;
        sendDesc.p_ndi_name = name_.c_str();
        sendDesc.p_groups = nullptr;
        sendDesc.clock_video = true;
        sendDesc.clock_audio = false;

        ndiSend_ = NDIlib_send_create(&sendDesc);
        if (!ndiSend_) {
            tc::logError() << "[NDISender] Failed to create NDI sender";
            return false;
        }

        pixelBuffer_.resize((size_t)width_ * height_ * 4);
        setup_ = true;

        tc::logNotice() << "[NDISender] Started: " << name_
                        << " (" << width_ << "x" << height_ << ")";
        return true;
    }

    void close() {
        if (ndiSend_) {
            // Synchronize async sends before destroying
            NDIlib_send_send_video_async_v2(ndiSend_, nullptr);
            NDIlib_send_destroy(ndiSend_);
            ndiSend_ = nullptr;
        }
        setup_ = false;
        pixelBuffer_.clear();
    }

    bool isSetup() const { return setup_; }

    // =========================================================================
    // Send Video
    // =========================================================================

    // Send raw RGBA pixel data
    bool send(const unsigned char* pixels, int width, int height) {
        if (!setup_ || !ndiSend_ || !pixels) return false;

        NDIlib_video_frame_v2_t frame;
        frame.xres = width;
        frame.yres = height;
        frame.FourCC = NDIlib_FourCC_type_RGBA;
        frame.frame_rate_N = fpsNumerator_;
        frame.frame_rate_D = fpsDenominator_;
        frame.picture_aspect_ratio = 0.0f;
        frame.frame_format_type = NDIlib_frame_format_type_progressive;
        frame.timecode = NDIlib_send_timecode_synthesize;
        frame.p_data = const_cast<uint8_t*>(pixels);
        frame.line_stride_in_bytes = width * 4;
        frame.p_metadata = nullptr;
        frame.timestamp = 0;

        if (async_) {
            NDIlib_send_send_video_async_v2(ndiSend_, &frame);
        } else {
            NDIlib_send_send_video_v2(ndiSend_, &frame);
        }

        return true;
    }

    // Send from Pixels object
    bool send(const tc::Pixels& pixels) {
        if (!pixels.isAllocated()) return false;
        return send(pixels.getData(), pixels.getWidth(), pixels.getHeight());
    }

    // Send from FBO (readback via Fbo::readPixels)
    bool send(tc::Fbo& fbo) {
        if (!setup_ || !fbo.isAllocated()) return false;

        int w = fbo.getWidth();
        int h = fbo.getHeight();

        size_t requiredSize = (size_t)w * h * 4;
        if (pixelBuffer_.size() != requiredSize) {
            pixelBuffer_.resize(requiredSize);
        }

        if (!fbo.readPixels(pixelBuffer_.data())) return false;
        return send(pixelBuffer_.data(), w, h);
    }

    // =========================================================================
    // Settings
    // =========================================================================

    void setFrameRate(float fps) {
        if (fps <= 0) return;

        // Exact representation for common frame rates
        if (std::abs(fps - 29.97f) < 0.1f) {
            fpsNumerator_ = 30000; fpsDenominator_ = 1001;
        } else if (std::abs(fps - 59.94f) < 0.1f) {
            fpsNumerator_ = 60000; fpsDenominator_ = 1001;
        } else if (std::abs(fps - 23.976f) < 0.1f) {
            fpsNumerator_ = 24000; fpsDenominator_ = 1001;
        } else {
            fpsNumerator_ = (int)(fps * 1000.0f);
            fpsDenominator_ = 1000;
        }
    }

    void setFrameRate(int numerator, int denominator) {
        if (numerator > 0 && denominator > 0) {
            fpsNumerator_ = numerator;
            fpsDenominator_ = denominator;
        }
    }

    float getFrameRate() const {
        return (fpsDenominator_ > 0) ? (float)fpsNumerator_ / (float)fpsDenominator_ : 0.0f;
    }

    void setAsync(bool async) { async_ = async; }
    bool isAsync() const { return async_; }

    // =========================================================================
    // Audio
    // =========================================================================

    void setAudioEnabled(bool enabled) { audioEnabled_ = enabled; }
    bool isAudioEnabled() const { return audioEnabled_; }
    void setAudioSampleRate(int sampleRate) { audioSampleRate_ = sampleRate; }
    void setAudioChannels(int channels) { audioChannels_ = channels; }

    // Send planar float audio (all ch0 samples, then all ch1, ...)
    bool sendAudio(const float* samples, int numSamples, int numChannels) {
        if (!setup_ || !ndiSend_ || !samples || !audioEnabled_) return false;

        NDIlib_audio_frame_v2_t audioFrame;
        audioFrame.sample_rate = audioSampleRate_;
        audioFrame.no_channels = numChannels;
        audioFrame.no_samples = numSamples;
        audioFrame.p_data = const_cast<float*>(samples);
        audioFrame.channel_stride_in_bytes = numSamples * sizeof(float);
        audioFrame.p_metadata = nullptr;
        audioFrame.timecode = NDIlib_send_timecode_synthesize;
        audioFrame.timestamp = 0;

        NDIlib_send_send_audio_v2(ndiSend_, &audioFrame);
        return true;
    }

    // =========================================================================
    // Info
    // =========================================================================

    std::string getName() const { return name_; }
    int getWidth() const { return width_; }
    int getHeight() const { return height_; }

    int getNumConnections() const {
        if (!ndiSend_) return 0;
        return NDIlib_send_get_no_connections(ndiSend_, 0);
    }

private:
    NDIlib_send_instance_t ndiSend_ = nullptr;
    std::string name_;
    int width_ = 0, height_ = 0;
    int fpsNumerator_ = 30000;
    int fpsDenominator_ = 1001;
    bool async_ = true;
    bool setup_ = false;
    bool audioEnabled_ = false;
    int audioSampleRate_ = 48000;
    int audioChannels_ = 2;
    std::vector<uint8_t> pixelBuffer_;
};

}  // namespace tcx::ndi

// -----------------------------------------------------------------------------
// Backward compatibility. The canonical namespace is now `tcx::ndi`. These
// silent aliases keep older code compiling: flat `tcx::NDISender` and legacy
// `trussc::NDISender`. DEPRECATED — removed in v1.0.0.
// (No [[deprecated]] attribute: under the usual `using namespace tc;` it would
//  warn on idiomatic unqualified use too. See tcxNDI README for migration.)
// -----------------------------------------------------------------------------
namespace tcx    { using ndi::NDISender; } // deprecated: remove at v1.0.0
namespace trussc { using tcx::ndi::NDISender; } // deprecated: remove at v1.0.0
