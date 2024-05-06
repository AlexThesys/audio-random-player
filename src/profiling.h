#pragma once

#include "Tracy.hpp"

#ifndef NDEBUG
#undef PROFILER_ENABLED
#endif // NDEBUG

#ifdef PROFILER_ENABLED
#define PROFILE_START(name) { \
ZoneScopedN((name))
#define PROFILE_STOP }
#define PROFILE_FRAME(name) FrameMarkNamed((name))
#define PROFILE_FRAME_START(name) FrameMarkStart((name))
#define PROFILE_FRAME_STOP(name) FrameMarkEnd((name))
#define PROFILE_SET_THREAD_NAME(name) tracy::SetThreadName((name))
#else
#define PROFILE_START(name)
#define PROFILE_STOP
#define PROFILE_FRAME(name)
#define PROFILE_FRAME_START(name)
#define PROFILE_FRAME_STOP(name)
#define PROFILE_SET_THREAD_NAME(name)
#endif // USE_PROFILER