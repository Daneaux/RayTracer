#pragma once

#include <Windows.h>
#include <cstring>

class InputManager {
public:

    enum Button {
        Left = 0,
        Right = 1,
        Middle = 2,
        None = -1
    };

    InputManager() {
        memset(m_keys, 0, sizeof(m_keys));
    }

    void OnKeyDown(WPARAM key) {
        if (key < 256) m_keys[key] = true;
    }

    void OnKeyUp(WPARAM key) {
        if (key < 256) m_keys[key] = false;
    }

    void OnMouseMove(int dx, int dy) {
        m_mouseDeltaX += dx;
        m_mouseDeltaY += dy;
    }

    void OnMouseWheel(int delta) {
        m_mouseWheelDelta += delta;
    }

    void ResetFrameDeltas() {
        m_mouseDeltaX = 0;
        m_mouseDeltaY = 0;
        m_mouseWheelDelta = 0;
    }

    bool IsKeyDown(int key) const {
        return (key >= 0 && key < 256) ? m_keys[key] : false;
    }

    int GetMouseDeltaX() const { return m_mouseDeltaX; }
    int GetMouseDeltaY() const { return m_mouseDeltaY; }
    int GetMouseWheelDelta() const { return m_mouseWheelDelta; }
	Button GetPressedButton() const { return m_pressedButton; }

    void SetCapture(HWND hwnd, Button button) {
        ::SetCapture(hwnd);
        ShowCursor(FALSE);
        m_captured = true;
        m_captureHwnd = hwnd;
		m_pressedButton = button;

        RECT rc;
        GetClientRect(hwnd, &rc);
        POINT center = { (rc.right) / 2, (rc.bottom) / 2 };
        ClientToScreen(hwnd, &center);
        SetCursorPos(center.x, center.y);
        m_lastCursorX = center.x;
        m_lastCursorY = center.y;
    }

    void ReleaseCapture() {
        if (m_captured) {
            ::ReleaseCapture();
            ShowCursor(TRUE);
            m_captured = false;
            m_captureHwnd = nullptr;
			m_pressedButton = None;
        }
    }

    bool IsCaptured() const { return m_captured; }
    HWND GetCaptureHwnd() const { return m_captureHwnd; }

    void OnRawMouseMove(int screenX, int screenY) {
        if (!m_captured) return;
        int dx = screenX - m_lastCursorX;
        int dy = screenY - m_lastCursorY;
        m_mouseDeltaX += dx;
        m_mouseDeltaY += dy;

        RECT rc;
        GetClientRect(m_captureHwnd, &rc);
        POINT center = { (rc.right) / 2, (rc.bottom) / 2 };
        ClientToScreen(m_captureHwnd, &center);
        SetCursorPos(center.x, center.y);
        m_lastCursorX = center.x;
        m_lastCursorY = center.y;
    }

private:
    bool m_keys[256] = {};
    int  m_mouseDeltaX = 0;
    int  m_mouseDeltaY = 0;
    int  m_mouseWheelDelta = 0;
    bool m_captured = false;
    HWND m_captureHwnd = nullptr;
    int  m_lastCursorX = 0;
    int  m_lastCursorY = 0;
    Button m_pressedButton = None;
};