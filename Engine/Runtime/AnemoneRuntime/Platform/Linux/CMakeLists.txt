if (LINUX)

target_sources(AnemoneRuntime
    PRIVATE
        "LinuxApplication.cxx"
        "LinuxApplication.hxx"

        "LinuxClipboard.cxx"

        "LinuxDebugger.cxx"

        "LinuxDialogs.cxx"

        "LinuxEnvironment.cxx"

        "LinuxFileHandle.cxx"
        "LinuxFileHandle.hxx"

        "LinuxFileSystem.cxx"

        "LinuxMemoryMappedFile.cxx"
        "LinuxMemoryMappedFile.hxx"

        "LinuxNamedMutex.cxx"
        "LinuxNamedMutex.hxx"

        "LinuxProcessorProperties.cxx"

        "LinuxPlatformTraceListeners.cxx"

        "LinuxProcess.cxx"
        "LinuxProcess.hxx"

        "LinuxSharedLibrary.cxx"
        "LinuxSharedLibrary.hxx"

        "LinuxStackTrace.cxx"

        "LinuxSystemAllocator.cxx"

        "LinuxWindow.cxx"
        "LinuxWindow.hxx"
)

endif()
