add_subdirectory("Platform")

target_sources(AnemoneRuntime
    PRIVATE
        "Clipboard.hxx"
        "Dialogs.hxx"
        "Environment.hxx"
        "ProcessorProperties.hxx"
)
