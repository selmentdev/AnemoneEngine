#pragma once
#include "AnemoneRuntime/Platform/Unix/UnixHeaders.hxx"
#include "AnemoneRuntime/UninitializedObject.hxx"

namespace Anemone::Private
{
    struct LinuxDialogsStatics final
    {
    };

    extern UninitializedObject<LinuxDialogsStatics> GDialogsStatics;
}
