target_sources(AnemoneRuntime
    PRIVATE
        #"DirectoryEnumerator.cxx"
        "ErrorCode.cxx"
)

if(WIN32)
    add_subdirectory("Windows")
elseif(ANDROID OR LINUX)
    add_subdirectory("Posix")
endif()
