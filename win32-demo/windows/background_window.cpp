#include "background_window.h"

#include <dwmapi.h>
#include <tchar.h>

namespace {

constexpr const wchar_t kWindowClassName[] = L"FLUTTER_RUNNER_BACKGROUND_WINDOW";
constexpr const wchar_t kWindowTitle[] = L"flutter_background_window";

#define SET_BACKGROUND_IMAGE_DWDATA    101
#define SET_BACKGROUND_COLOR_DWDATA    102



using EnableNonClientDpiScaling = BOOL __stdcall(HWND hwnd);

// Scale helper to convert logical scaler values to physical using passed in
// scale factor
int Scale(int source, double scale_factor) {
    return static_cast<int>(source * scale_factor);
}

// Dynamically loads the |EnableNonClientDpiScaling| from the User32 module.
// This API is only needed for PerMonitor V1 awareness mode.
void EnableFullDpiSupportIfAvailable(HWND hwnd) {
    HMODULE user32_module = LoadLibraryA("User32.dll");
    if (!user32_module) {
        return;
    }
    auto enable_non_client_dpi_scaling =
        reinterpret_cast<EnableNonClientDpiScaling*>(
            GetProcAddress(user32_module, "EnableNonClientDpiScaling"));
    if (enable_non_client_dpi_scaling != nullptr) {
        enable_non_client_dpi_scaling(hwnd);
    }
    FreeLibrary(user32_module);
}

}  // namespace


BackgroundWindow::BackgroundWindow()
{
}

BackgroundWindow::~BackgroundWindow()
{
    Destroy();
}

bool BackgroundWindow::Create(HINSTANCE hInstance)
{
    Destroy();

    if (!RegisterWindowClass(hInstance))
        return false;

    const POINT origin = { 10,10 };
    const SIZE size = { 1280,720 };
    HMONITOR monitor = MonitorFromPoint(origin, MONITOR_DEFAULTTONEAREST);
    //UINT dpi = FlutterDesktopGetDpiForMonitor(monitor);
    double scale_factor = 1.25;// dpi / 96.0;

    HWND window = CreateWindow(
        kWindowClassName, kWindowTitle, WS_POPUP,
        Scale(origin.x, scale_factor), Scale(origin.y, scale_factor),
        Scale(size.cx, scale_factor), Scale(size.cy, scale_factor),
        nullptr, nullptr, GetModuleHandle(nullptr), this);

    if (!window) {
        return false;
    }
    _windowHandle = window;

    ShowWindow(window, SW_HIDE); //默认隐藏
    UpdateWindow(window);

    ::SetWindowPos(window, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

    return true;
}

void BackgroundWindow::UpdateBackground(const std::wstring& path)
{
    _bgImagePath = path;
    //窗口显示时刷新窗口，否则只更新背景图片路径
    if (IsWindowVisible(_windowHandle))
    {
        RECT rc;
        GetClientRect(_windowHandle, &rc);
        InvalidateRect(_windowHandle, &rc, false);
        UpdateWindow(_windowHandle);
    }
}

BackgroundWindow* BackgroundWindow::GetThisFromHandle(HWND const window) noexcept
{
    return reinterpret_cast<BackgroundWindow*>(GetWindowLongPtr(window, GWLP_USERDATA));
}

LRESULT BackgroundWindow::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) noexcept
{
    if (message == WM_NCCREATE)
    {
        auto window_struct = reinterpret_cast<CREATESTRUCT*>(lParam);
        SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(window_struct->lpCreateParams));

        auto that = static_cast<BackgroundWindow*>(window_struct->lpCreateParams);
        EnableFullDpiSupportIfAvailable(hWnd);
        that->_windowHandle = hWnd;
    }
    else if (BackgroundWindow* that = GetThisFromHandle(hWnd)) {
        return that->MessageHandler(hWnd, message, wParam, lParam);
    }
    return DefWindowProc(hWnd, message, wParam, lParam);
}

LRESULT BackgroundWindow::MessageHandler(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_COPYDATA:
    {
        COPYDATASTRUCT* pCopyData = reinterpret_cast<COPYDATASTRUCT*>(lParam);
        if (pCopyData != NULL && pCopyData->dwData == SET_BACKGROUND_IMAGE_DWDATA)
        {
            // TODO: 处理pCopyData->lpData指向的数据
            std::wstring path((wchar_t*)pCopyData->lpData, pCopyData->cbData / sizeof(wchar_t));
            UpdateBackground(path);
        }
    }
    break;
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
        // TODO: 在此处添加使用 hdc 的任何绘图代码...
        if (!_bgImagePath.empty())
        {
            RECT rc;
            GetClientRect(hWnd, &rc);
            // LoadImage只能加载bmp位图
            HBITMAP hBitmap = (HBITMAP)LoadImage(NULL, _bgImagePath.c_str(), IMAGE_BITMAP, rc.right - rc.left, rc.bottom - rc.top, LR_LOADFROMFILE);
            HDC hMemDc = CreateCompatibleDC(hdc);
            SelectObject(hMemDc, hBitmap);
            BitBlt(hdc, 0, 0, rc.right - rc.left, rc.bottom - rc.top, hMemDc, 0, 0, SRCCOPY);
        }
        EndPaint(hWnd, &ps);
    }
    break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

bool BackgroundWindow::RegisterWindowClass(HINSTANCE hInstance)
{
    if (_classRegistered)
        return true;

    WNDCLASSEXW wcex;
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = 0;
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = nullptr;
    wcex.lpszClassName = kWindowClassName;
    wcex.hIconSm = 0;
    auto ret = RegisterClassEx(&wcex);
    if (!ret)
    {
        //注册失败
        return false;
    }
    _classRegistered = true;
    return true;
}

void BackgroundWindow::UnregisterWindowClass()
{
    UnregisterClass(kWindowClassName, nullptr);
}

void BackgroundWindow::Destroy()
{
    if (_windowHandle)
    {
        DestroyWindow(_windowHandle);
        _windowHandle = nullptr;
    }
    if (_classRegistered)
    {
        UnregisterWindowClass();
    }
}
