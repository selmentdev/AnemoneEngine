add_subdirectory("Linux")
add_subdirectory("Windows")

target_sources(AnemoneRuntime
    PRIVATE
        "Application.hxx"
        "Clipboard.hxx"
        "Dialogs.hxx"
        "Environment.hxx"
        "ProcessorProperties.hxx"
)
