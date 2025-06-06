#include "AnemoneRuntime/App/Platform/Windows/SplashScreen.hxx"
#include "AnemoneRuntime/Base/UninitializedObject.hxx"

namespace Anemone
{
    namespace
    {
        UninitializedObject<SplashScreen> GSplashScreen{};
    }

    void ISplashScreen::Initialize()
    {
        GSplashScreen.Create();
    }

    void ISplashScreen::Finalize()
    {
        GSplashScreen.Destroy();
    }

    ISplashScreen& ISplashScreen::Get()
    {
        return *GSplashScreen;
    }
}

namespace Anemone
{
    SplashScreen::SplashScreen()
    {
        this->_sync.Attach(
            CreateEventW(
                nullptr,
                FALSE,
                FALSE,
                nullptr));

        this->_thread.Attach(
            CreateThread(
                nullptr,
                0,
                &SplashScreen::ThreadProc,
                this,
                0,
                nullptr));
    }

    SplashScreen::~SplashScreen()
    {
        WaitForSingleObject(this->_sync.Get(), INFINITE);

        if (this->_window)
        {
            PostMessageW(this->_window, WM_CLOSE, 0, 0);
        }

        WaitForSingleObject(this->_thread.Get(), INFINITE);

        this->_thread = {};
        this->_sync = {};
    }

    void SplashScreen::SetText(std::string_view text)
    {
        UniqueLock scope{this->_lock};
        Interop::Windows::WidenString(this->_text, text);

        InvalidateRect(this->_window, nullptr, FALSE);
    }

    void SplashScreen::SetProgress(float progress)
    {
        UniqueLock scope{this->_lock};
        this->_progress = progress;

        InvalidateRect(this->_window, nullptr, FALSE);
    }

    LRESULT CALLBACK SplashScreen::WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
    {
        SplashScreen* self = nullptr;

        if (msg == WM_NCCREATE)
        {
            auto const create = std::bit_cast<LPCREATESTRUCTW>(lparam);
            self = std::bit_cast<SplashScreen*>(create->lpCreateParams);
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        }
        else
        {
            self = std::bit_cast<SplashScreen*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
        }

        switch (msg)
        {
        case WM_PAINT:
            {
                PAINTSTRUCT ps{};
                HDC hdc = BeginPaint(hwnd, &ps);

                RECT rect{};
                GetClientRect(hwnd, &rect);
                FillRect(hdc, &rect, reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1));

                if (self)
                {
                    HFONT SystemFontHandle = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
                    UniqueLock scope{self->_lock};

                    SelectObject(hdc, SystemFontHandle);
                    SetTextColor(hdc, RGB(0, 0, 0));
                    SetBkMode(hdc, TRANSPARENT);
                    DrawTextW(
                        hdc,
                        self->_text.c_str(),
                        -1,
                        &rect,
                        DT_CENTER | DT_VCENTER | DT_SINGLELINE);
                }

                EndPaint(hwnd, &ps);
                break;
            }

        case WM_ERASEBKGND:
            {
                return 1;
            }

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;

        case WM_NCHITTEST:
            {
                LRESULT result = DefWindowProcW(hwnd, msg, wparam, lparam);

                if (result == HTCLIENT)
                {
                    result = HTCAPTION;
                }

                return result;
            }

        default:
            break;
        }

        return DefWindowProcW(hwnd, msg, wparam, lparam);
    }

    DWORD SplashScreen::ThreadProc(LPVOID lpParameter)
    {
        SplashScreen* self = static_cast<SplashScreen*>(lpParameter);

        HMODULE hInstance = GetModuleHandleW(nullptr);

        WNDCLASSEXW wcex{};
        wcex.cbSize = sizeof(wcex);
        wcex.style = 0;
        wcex.lpfnWndProc = WindowProc;
        wcex.cbClsExtra = 0;
        wcex.cbWndExtra = 0;
        wcex.hInstance = hInstance;
        wcex.hIcon = LoadIconW(nullptr, IDI_APPLICATION);
        wcex.hCursor = LoadCursorW(nullptr, IDC_ARROW);
        wcex.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
        wcex.lpszMenuName = nullptr;
        wcex.lpszClassName = L"AnemoneSplashScreen";
        wcex.hIconSm = LoadIconW(nullptr, IDI_APPLICATION);

        self->_class = RegisterClassExW(&wcex);

        if (not self->_class)
        {
            return 0;
        }

        int const width = 640;
        int const height = 480;

        int const centerX = GetSystemMetrics(SM_CXSCREEN) / 2;
        int const centerY = GetSystemMetrics(SM_CYSCREEN) / 2;

        int const positionX = centerX - width / 2;
        int const positionY = centerY - height / 2;

        self->_window = CreateWindowExW(
            WS_EX_TOOLWINDOW,
            MAKEINTATOM(self->_class),
            L"Anemone",
            WS_POPUP,
            positionX,
            positionY,
            width,
            height,
            nullptr,
            nullptr,
            hInstance,
            self);

        SetEvent(self->_sync.Get());

        if (self->_window)
        {
            //SetLayeredWindowAttributes(self->_window, 0, 0x44, LWA_ALPHA);

            ShowWindow(self->_window, SW_SHOW);
            UpdateWindow(self->_window);

            MSG msg{};

            while (GetMessageW(&msg, nullptr, 0, 0))
            {
                TranslateMessage(&msg);
                DispatchMessageW(&msg);
            }
        }

        UnregisterClassW(MAKEINTATOM(self->_class), hInstance);

        return 0;
    }
}
