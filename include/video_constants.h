// video_constants.h - resolutions and video constants
#pragma once
namespace Video {
struct Resolution { int w; int h; };
inline constexpr Resolution RESOLUTIONS[] = {
    {480,800},{540,960},{600,1000},{720,1280},{800,1440},{900,1600},{1080,1920}
};
inline constexpr int RESOLUTION_COUNT = (int)(sizeof(RESOLUTIONS)/sizeof(RESOLUTIONS[0]));
}
