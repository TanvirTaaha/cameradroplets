#pragma once
#ifndef __FFMPEG_H__
#define __FFMPEG_H__

#include <iostream>
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/dict.h>
}

namespace camdrops {
// Function to open the RTSP link with your identical parameters
AVFormatContext* open_rtsp_stream(const std::string& rtsp_url, int timeout_seconds) {
  AVFormatContext* format_ctx = avformat_alloc_context();
  AVDictionary* options = nullptr;

  // 1. -rtsp_transport tcp
  av_dict_set(&options, "rtsp_transport", "tcp", 0);

  // 2. -fflags nobuffer+discardcorrupt
  av_dict_set(&options, "fflags", "nobuffer+discardcorrupt", 0);

  // 3. -flags low_delay
  av_dict_set(&options, "flags", "low_delay", 0);

  // 4. -max_delay 0
  av_dict_set(&options, "max_delay", "0", 0);

  // 5. -probesize 1048576 (1MB)
  av_dict_set(&options, "probesize", "1048576", 0);

  // 6. -analyzeduration 0
  av_dict_set(&options, "analyzeduration", "0", 0);

  // 7. -timeout (converted from seconds to microseconds)
  int64_t timeout_us = static_cast<int64_t>(timeout_seconds) * 1000000;
  av_dict_set(&options, "timeout", std::to_string(timeout_us).c_str(), 0);

  // Open the stream natively
  if (avformat_open_input(&format_ctx, rtsp_url.c_str(), nullptr, &options) < 0) {
    std::cerr << "Could not open RTSP stream target." << std::endl;
    av_dict_free(&options);
    return nullptr;
  }

  // Free the options dictionary after opening
  av_dict_free(&options);
  return format_ctx;
}
}  // namespace camdrops

#endif  // __FFMPEG_H__