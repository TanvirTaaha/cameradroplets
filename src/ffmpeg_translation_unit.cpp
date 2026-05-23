#include <camdrops/ffmpeg.h>

// Keeps FFmpeg headers in an actual translation unit so compile_commands.json
// includes a command entry that IntelliSense can use for this header.
namespace camdrops {
void ffmpeg_header_translation_unit_anchor() {}
}  // namespace camdrops
