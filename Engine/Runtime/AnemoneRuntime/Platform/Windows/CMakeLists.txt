if (WIN32)

target_sources(AnemoneRuntime
    PRIVATE
        "WindowsApplication.cxx"
        "WindowsApplication.hxx"

        "WindowsCrashHandler.hxx"

        "WindowsError.cxx"
        "WindowsError.hxx"

        "WindowsFileHandle.cxx"
        "WindowsFileHandle.hxx"

        "WindowsFileSystem.cxx"

        "WindowsHeaders.hxx"

        "WindowsInput.cxx"
        "WindowsInput.hxx"

        "WindowsInterop.hxx"

        "WindowsMemoryMappedFile.cxx"
        "WindowsMemoryMappedFile.hxx"

        "WindowsNamedMutex.cxx"
        "WindowsNamedMutex.hxx"

        "WindowsProcess.cxx"
        "WindowsProcess.hxx"

        "WindowsSharedLibrary.cxx"
        "WindowsSharedLibrary.hxx"

        "WindowsSplashScreen.cxx"
        "WindowsSplashScreen.hxx"

        "WindowsStackTrace.cxx"

        "WindowsSystemAllocator.cxx"

        "WindowsWindow.cxx"
        "WindowsWindow.hxx"
)

endif()
