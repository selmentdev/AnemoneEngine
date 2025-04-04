#pragma once
#include "AnemoneRuntime/Platform/Base/BaseHeaders.hxx"

#if ANEMONE_PLATFORM_WINDOWS
#include "AnemoneRuntime/Diagnostics/Platform/Windows/WindowsDebugger.hxx"
#elif ANEMONE_PLATFORM_LINUX
#include "AnemoneRuntime/Diagnostics/Platform/Linux/LinuxDebugger.hxx"
#else
#error "Not supported"
#endif
