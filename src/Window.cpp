#include "Window.h"
#include "Application.h"

Window::~Window() {
    Destroy();
}

bool Window::Create(HINSTANCE hInst, const wchar_t* title,
                    uint32_t width, uint32_t height, Application* app) {
    m_app = app;
    m_width = width;
    m_height = height;

    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(wc);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInst;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.lpszClassName = title;
    RegisterClassExW(&wc);

    RECT rc = {0, 0, (LONG)width, (LONG)height};
    AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);

    m_hwnd = CreateWindowExW(
        0, title, title,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        rc.right - rc.left, rc.bottom - rc.top,
        nullptr, nullptr, hInst, this
    );

    if (!m_hwnd) return false;

    ShowWindow(m_hwnd, SW_SHOW);
    UpdateWindow(m_hwnd);
    m_alive = true;
    return true;
}

void Window::Destroy() {
    if (m_hwnd) {
        DestroyWindow(m_hwnd);
        m_hwnd = nullptr;
    }
    m_alive = false;
}

bool Window::ConsumeResizeFlag() {
    if (m_resized) {
        m_resized = false;
        return true;
    }
    return false;
}

LRESULT CALLBACK Window::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_NCCREATE) {
        auto* cs = reinterpret_cast<CREATESTRUCT*>(lParam);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(cs->lpCreateParams));
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }

    auto* self = reinterpret_cast<Window*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    if (!self) return DefWindowProc(hwnd, msg, wParam, lParam);

    switch (msg) {
    case WM_SIZE:
        if (wParam != SIZE_MINIMIZED) {
            self->m_width = LOWORD(lParam);
            self->m_height = HIWORD(lParam);
            self->m_resized = true;
        }
        return 0;

    case WM_DESTROY:
        self->m_alive = false;
        PostQuitMessage(0);
        return 0;

    case WM_KEYDOWN:
    case WM_KEYUP:
    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
    case WM_RBUTTONDOWN:
    case WM_RBUTTONUP:
    case WM_MBUTTONDOWN:
    case WM_MBUTTONUP:
    case WM_MOUSEMOVE:
    case WM_MOUSEWHEEL:
        if (self->m_app) {
            self->m_app->OnWindowMessage(hwnd, msg, wParam, lParam);
        }
        return 0;
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}
