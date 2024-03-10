#pragma once

#include "Tracy.hpp"

#ifdef NDEBUG
#define USE_PROFILER
#endif // NDEBUG

#ifdef USE_PROFILER
#define PROFILE_START(name) { \
ZoneScopedN((name))
#define PROFILE_STOP }
#define PROFILE_FRAME(name) FrameMarkNamed((name))
#define PROFILE_FRAME_START(name) FrameMarkStart((name))
#define PROFILE_FRAME_STOP(name) FrameMarkEnd((name))
#else
#define PROFILE_START(name)
#define PROFILE_STOP
#define PROFILE_FRAME(name)
#define PROFILE_FRAME_START(name)
#define PROFILE_FRAME_STOP(name)
#endif // USE_PROFILER