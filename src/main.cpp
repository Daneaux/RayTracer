#include "Application.h"
#include <Windows.h>

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int) {
    Application app;
    if (!app.Initialize(hInstance)) {
        MessageBoxA(nullptr, "Failed to initialize application", "Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    MSG msg = {};
    while (app.IsRunning()) {
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) {
                app.Shutdown();
                return (int)msg.wParam;
            }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        app.Tick();
    }

    app.Shutdown();
    return 0;
}
