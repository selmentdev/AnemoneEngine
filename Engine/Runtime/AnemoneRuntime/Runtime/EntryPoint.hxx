#pragma once
#include "AnemoneRuntime/Interop/Headers.hxx"

// GPU power management markers.
extern "C"
{
    // NVIDIA: http://developer.download.nvidia.com/devzone/devcenter/gamegraphics/files/OptimusRenderingPolicies.pdf
    // ReSharper disable once CppNonInlineVariableDefinitionInHeaderFile
    ANEMONE_DLLEXPORT unsigned NvOptimusEnablement = 0x00000001;

    // AMD: http://devgurus.amd.com/thread/169965
    // ReSharper disable once CppNonInlineVariableDefinitionInHeaderFile
    ANEMONE_DLLEXPORT int AmdPowerXpressRequestHighPerformance = 1;
}

#if !defined(ANEMONE_APPLICATION)
#error "This header must be included from the application"
#endif

extern "C" RUNTIME_API void AnemoneRuntimeInitialize(
    int argc,
    char** argv,
    bool console);

extern "C" RUNTIME_API void AnemoneRuntimeFinalize();

int AnemoneMain(int argc, char** argv);

#if ANEMONE_PLATFORM_WINDOWS && defined(ANEMONE_APPLICATION_UI)

#include "AnemoneRuntime/Interop/Windows/Headers.hxx"

// ReSharper disable once CppNonInlineFunctionDefinitionInHeaderFile
int WINAPI WinMain(
    [[maybe_unused]] _In_ HINSTANCE hInstance,
    [[maybe_unused]] _In_opt_ HINSTANCE hPrevInstance,
    [[maybe_unused]] _In_ LPSTR lpCmdLine,
    [[maybe_unused]] _In_ int nShowCmd)
{
    int argc = __argc;
    char** argv = __argv;

    AnemoneRuntimeInitialize(argc, argv, false);

    int const result = AnemoneMain(argc, argv);

    AnemoneRuntimeFinalize();

    return result;
}

#else

int main(int argc, char** argv)
{
#if defined(ANEMONE_APPLICATION_CONSOLE)
    constexpr bool console = true;
#else
    constexpr bool console = false;
#endif

    AnemoneRuntimeInitialize(argc, argv, console);

    int const result = AnemoneMain(argc, argv);

    AnemoneRuntimeFinalize();

    return result;
}

#endif
