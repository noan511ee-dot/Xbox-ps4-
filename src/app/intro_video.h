// SPDX-License-Identifier: GPL-3.0-or-later
// Start-up intro: a short H.264 clip with its sound, played full screen before
// the menus. The clip is stored as a raw Annex-B stream (assets/intro/intro.h264)
// plus 48 kHz stereo PCM (assets/intro/intro.pcm), so no container demuxer is
// needed on the console. Any problem (missing file, decoder error, slow start)
// ends the intro at once: it must never keep the app from starting.
#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "media/decoder.h"

namespace xc::app {

class IntroVideo {
public:
    IntroVideo() = default;
    ~IntroVideo();
    IntroVideo(const IntroVideo&) = delete;
    IntroVideo& operator=(const IntroVideo&) = delete;

    // Loads <assetDir>/intro/intro.h264 and intro.pcm. False: nothing to play.
    bool start(const std::string& assetDir);
    // One pass of the main loop: decodes and shows the frames that are due,
    // feeds the sound. True while the intro is still running.
    bool step(uint64_t nowMs);
    // The viewer pressed a button: stop now.
    void skip();
    bool active() const { return active_; }
    // Frames shown so far (tests, logs).
    int framesShown() const { return shown_; }

private:
    void finish(const char* why);

    bool active_ = false;
    bool audioOn_ = false;
    std::vector<uint8_t> stream_;
    std::vector<std::pair<size_t, size_t>> units_;
    std::vector<int16_t> pcm_;  // interleaved stereo
    std::unique_ptr<media::VideoDecoder> decoder_;
    size_t next_ = 0;           // next access unit to decode
    size_t audioPos_ = 0;       // frames (stereo pairs) already queued
    uint64_t startMs_ = 0;
    uint64_t lastShownMs_ = 0;
    int shown_ = 0;
    int dropped_ = 0;
};

}  // namespace xc::app
