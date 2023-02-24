#ifndef RUNNER_BACKGROUND_WINDOW_H_
#define RUNNER_BACKGROUND_WINDOW_H_

#include <windows.h>

#include <functional>
#include <memory>
#include <string>


class BackgroundWindow
{
public:
    BackgroundWindow();
    ~BackgroundWindow();

    bool Create(HINSTANCE hInstance);

    void UpdateBackground(const std::wstring& path);

    HWND GetHandle() { return _windowHandle; }

private:
    static BackgroundWindow* GetThisFromHandle(HWND const window) noexcept;
    static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) noexcept;
    LRESULT CALLBACK MessageHandler(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

    bool RegisterWindowClass(HINSTANCE hInstance);
    void UnregisterWindowClass();
    void Destroy();

private:
    HWND _windowHandle = nullptr;
    bool _classRegistered = false;
    std::wstring _bgImagePath;
};


#endif  // RUNNER_WIN32_WINDOW_H_
