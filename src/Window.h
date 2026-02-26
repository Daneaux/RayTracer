#pragma once

#include <Windows.h>
#include <cstdint>

class Application;

class Window {
public:
    Window() = default;
    ~Window();

    bool Create(HINSTANCE hInst, const wchar_t* title,
                uint32_t width, uint32_t height, Application* app);
    void Destroy();

    HWND     GetHWND() const { return m_hwnd; }
    uint32_t GetWidth() const { return m_width; }
    uint32_t GetHeight() const { return m_height; }
    bool     ConsumeResizeFlag();
    bool     IsAlive() const { return m_alive; }

    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

private:
    HWND         m_hwnd = nullptr;
    uint32_t     m_width = 0;
    uint32_t     m_height = 0;
    bool         m_resized = false;
    bool         m_alive = false;
    Application* m_app = nullptr;
};
