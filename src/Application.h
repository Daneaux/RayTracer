#pragma once

#include "DXDevice.h"
#include "Window.h"
#include "SwapChainTarget.h"
#include "Camera.h"
#include "InputManager.h"
#include "Scene.h"
#include "renderer/IRenderer.h"
#include "renderer/SceneOverviewRenderer.h"
#include "renderer/SoftwareOverviewRenderer.h"
#include <memory>
#include <Windows.h>

class Application {
public:
    bool Initialize(HINSTANCE hInst);
    void Tick();
    void Shutdown();
    void OnWindowMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    bool IsRunning() const;

private:
    std::unique_ptr<DXDevice>               m_device;
    std::unique_ptr<Window>                 m_window1;
    std::unique_ptr<Window>                 m_window2;
    std::unique_ptr<SwapChainTarget>        m_target1;
    std::unique_ptr<SwapChainTarget>        m_target2;
    std::unique_ptr<IRenderer>              m_renderer;          // window 1
    std::unique_ptr<SoftwareOverviewRenderer> m_overviewRenderer;  // window 2
    std::unique_ptr<FlyCamera>              m_camera1;
    std::unique_ptr<FlyCamera>              m_camera2;
    InputManager                            m_input1;
    InputManager                            m_input2;
    Scene                                   m_scene;

    LARGE_INTEGER m_frequency = {};
    LARGE_INTEGER m_lastTime = {};
    float         m_deltaTime = 0.0f;
    float         m_fovY = 0.7854f; // 45 degrees
};
