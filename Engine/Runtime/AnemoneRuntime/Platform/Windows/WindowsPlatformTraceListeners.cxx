#include "AnemoneRuntime/Platform/PlatformTraceListeners.hxx"
#include "AnemoneRuntime/Platform/Windows/WindowsInterop.hxx"
#include "AnemoneRuntime/Platform/Windows/WindowsEnvironment.hxx"
#include "AnemoneRuntime/UninitializedObject.hxx"
#include "AnemoneRuntime/Diagnostics/Trace.hxx"
#include "AnemoneRuntime/Diagnostics/ConsoleTraceListener.hxx"

#include <winmeta.h>
#include <TraceLoggingProvider.h>

namespace Anemone
{
    class WindowsDebugTraceListener final : public TraceListener
    {
    public:
        void TraceEvent(TraceLevel level, const char* message, size_t size) override
        {
            (void)level;

            Interop::win32_OutputDebugString(message, size + 1);
            Interop::win32_OutputDebugString("\r\n", 3);
        }
    };

    static UninitializedObject<WindowsDebugTraceListener> GWindowsDebugTraceListener;

    // C0B8BB2A-7E1E-5A0B-0ACD-3EE6187895ED
    // https://learn.microsoft.com/en-us/windows/win32/api/traceloggingprovider/
    TRACELOGGING_DEFINE_PROVIDER(
        GWindowsEtwTraceProvider,
        "AnemoneEngine.Runtime",
        (0xC0B8BB2A, 0x7E1E, 0x5A0B, 0x0A, 0xCD, 0x3E, 0xE6, 0x18, 0x78, 0x95, 0xED));


    class WindowsEtwTraceListener final : public TraceListener
    {
    public:
        WindowsEtwTraceListener()
        {
            TraceLoggingRegister(GWindowsEtwTraceProvider);
        }

        ~WindowsEtwTraceListener() override
        {
            TraceLoggingUnregister(GWindowsEtwTraceProvider);
        }

        void TraceEvent(TraceLevel level, const char* message, size_t size) override
        {
            switch (level)
            {
            case TraceLevel::Verbose:
            case TraceLevel::Debug:
                TraceLoggingWrite(
                    GWindowsEtwTraceProvider,
                    "Log",
                    TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE),
                    TraceLoggingCountedUtf8String(message, static_cast<ULONG>(size), "Message"));
                break;

            case TraceLevel::Information:
                TraceLoggingWrite(
                    GWindowsEtwTraceProvider,
                    "Log",
                    TraceLoggingLevel(WINEVENT_LEVEL_INFO),
                    TraceLoggingCountedUtf8String(message, static_cast<ULONG>(size), "Message"));
                break;

            case TraceLevel::Warning:
                TraceLoggingWrite(
                    GWindowsEtwTraceProvider,
                    "Log",
                    TraceLoggingLevel(WINEVENT_LEVEL_WARNING),
                    TraceLoggingCountedUtf8String(message, static_cast<ULONG>(size), "Message"));

                break;
            case TraceLevel::Error:
                TraceLoggingWrite(
                    GWindowsEtwTraceProvider,
                    "Log",
                    TraceLoggingLevel(WINEVENT_LEVEL_ERROR),
                    TraceLoggingCountedUtf8String(message, static_cast<ULONG>(size), "Message"));
                break;

            case TraceLevel::Fatal:
                TraceLoggingWrite(
                    GWindowsEtwTraceProvider,
                    "Log",
                    TraceLoggingLevel(WINEVENT_LEVEL_CRITICAL),
                    TraceLoggingCountedUtf8String(message, static_cast<ULONG>(size), "Message"));
                break;

            case TraceLevel::None:
                break;
            }
        }
    };

    static UninitializedObject<WindowsEtwTraceListener> GWindowsEtwTraceListener;

    static UninitializedObject<ConsoleTraceListener> GConsoleTraceListener;

    void PlatformTraceListeners::Initialize()
    {
        GWindowsEtwTraceListener.Create();
        Trace::AddListener(*GWindowsEtwTraceListener);

        GWindowsDebugTraceListener.Create();
        Trace::AddListener(*GWindowsDebugTraceListener);

        // TODO: Move this to application setup. It should be common to all platforms and configurable.
        if (WindowsEnvironment::IsConsoleApplication())
        {
            GConsoleTraceListener.Create();
            Trace::AddListener(*GConsoleTraceListener);
        }
    }

    void PlatformTraceListeners::Finalize()
    {
        if (WindowsEnvironment::IsConsoleApplication())
        {
            Trace::RemoveListener(*GConsoleTraceListener);
            GConsoleTraceListener.Destroy();
        }

        Trace::RemoveListener(*GWindowsDebugTraceListener);
        GWindowsDebugTraceListener.Destroy();

        Trace::RemoveListener(*GWindowsEtwTraceListener);
        GWindowsEtwTraceListener.Destroy();
    }
}
