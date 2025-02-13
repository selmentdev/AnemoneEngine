#pragma once
#include "AnemoneRuntime/Platform/Base/BaseHeaders.hxx"
#include "AnemoneRuntime/ErrorCode.hxx"

#if ANEMONE_PLATFORM_WINDOWS
#include "AnemoneRuntime/Platform/Windows/WindowsSharedLibrary.hxx"
#elif ANEMONE_PLATFORM_LINUX
#include "AnemoneRuntime/Platform/Linux/LinuxSharedLibrary.hxx"
#elif ANEMONE_PLATFORM_ANDROID
#include "AnemoneRuntime/Platform/Android/AndroidSharedLibrary.hxx"
#else
#error "Unsupported platform"
#endif
