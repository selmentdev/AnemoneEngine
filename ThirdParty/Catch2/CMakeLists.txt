# https://github.com/catchorg/Catch2/releases/tag/v3.7.0
anemone_add_sdk(SdkCatch2 STATIC)

if(MSVC)
# TODO: Remove this once Catch2 is fixed
target_compile_definitions(SdkCatch2 PUBLIC "CATCH_CONFIG_NO_MSVC_UMUL128")
endif()

target_compile_definitions(SdkCatch2 PUBLIC "CATCH_AMALGAMATED_CUSTOM_MAIN")

add_subdirectory("source")
