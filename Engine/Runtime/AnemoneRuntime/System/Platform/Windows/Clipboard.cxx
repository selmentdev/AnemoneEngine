#include "AnemoneRuntime/System/Clipboard.hxx"

#include "AnemoneRuntime/Base/UninitializedObject.hxx"
#include "AnemoneRuntime/Interop/Windows/Text.hxx"

namespace Anemone
{
    namespace
    {
        struct ClipboardStatics final
        {
            UINT binaryClipboardFormat = 0;

            explicit ClipboardStatics()
            {
                this->binaryClipboardFormat = RegisterClipboardFormatW(L"AnemoneEngineBinaryData");
            }

            ClipboardStatics(ClipboardStatics const&) = delete;

            ClipboardStatics(ClipboardStatics&&) = delete;

            ClipboardStatics& operator=(ClipboardStatics const&) = delete;

            ClipboardStatics& operator=(ClipboardStatics&&) = delete;

            ~ClipboardStatics()
            {
                this->binaryClipboardFormat = 0;
            }
        };

        UninitializedObject<ClipboardStatics> gClipboardStatics{};
    }

    namespace Internal
    {
        extern void InitializeClipboard()
        {
            gClipboardStatics.Create();
        }

        extern void FinalizeClipboard()
        {
            gClipboardStatics.Destroy();
        }
    }
}

namespace Anemone
{
    void Clipboard::Clear()
    {
        if (OpenClipboard(nullptr))
        {
            EmptyClipboard();
            CloseClipboard();
        }
    }

    bool Clipboard::GetText(std::string& result)
    {
        result.clear();

        bool success = false;

        if (OpenClipboard(nullptr))
        {
            if (HGLOBAL const hUnicodeText = GetClipboardData(CF_UNICODETEXT); hUnicodeText != nullptr)
            {
                if (wchar_t const* const data = static_cast<wchar_t const*>(GlobalLock(hUnicodeText)))
                {
                    Interop::Windows::NarrowString(result, data);
                    GlobalUnlock(hUnicodeText);
                    success = true;
                }
            }
            else if (HGLOBAL const hText = GetClipboardData(CF_TEXT); hText != nullptr)
            {
                if (char const* const data = static_cast<char const*>(GlobalLock(hText)))
                {
                    result = data;
                    GlobalUnlock(hText);
                    success = true;
                }
            }

            CloseClipboard();
        }

        return success;
    }

    bool Clipboard::SetText(std::string_view value)
    {
        bool success = false;

        if (OpenClipboard(nullptr))
        {
            EmptyClipboard();

            Interop::string_buffer<wchar_t, 512> buffer{};
            Interop::Windows::WidenString(buffer, value);
            std::wstring_view const data = buffer.as_view();

            size_t const size = data.size() + 1; // Include null terminator.

            if (HGLOBAL const handle = GlobalAlloc(GMEM_MOVEABLE, size * sizeof(wchar_t)); handle != nullptr)
            {
                if (void* const destination = GlobalLock(handle); destination != nullptr)
                {
                    // Copy zero terminated string.
                    std::memcpy(destination, data.data(), size * sizeof(wchar_t));
                    GlobalUnlock(handle);

                    if (SetClipboardData(CF_UNICODETEXT, handle) != nullptr)
                    {
                        success = true;
                    }
                }
            }

            CloseClipboard();
        }

        return success;
    }
}
