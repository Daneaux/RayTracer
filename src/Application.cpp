#include "Application.h"
#include "renderer/SoftwareRenderer.h"
#include "renderer/GPURenderer.h"
#include "renderer/RenderFactory.h"


bool Application::Initialize(HINSTANCE hInst) {
    // Timing
    QueryPerformanceFrequency(&m_frequency);
    QueryPerformanceCounter(&m_lastTime);
    SetupScene();

    // D3D11 device
    m_device = std::make_unique<DXDevice>();
    if (!m_device->Initialize()) {
        OutputDebugStringA("Failed to initialize D3D11 device\n");
        return false;
    }

    // Window 1 - Scene View (static)
    m_window1 = std::make_unique<Window>();
    if (!m_window1->Create(hInst, L"RayTrace2 - Scene View", 800, 600, this)) {
        OutputDebugStringA("Failed to create window 1\n");
        return false;
    }

    // Window 2 - Navigator (fly camera)
    m_window2 = std::make_unique<Window>();
    if (!m_window2->Create(hInst, L"RayTrace2 - Navigator", 800, 600, this)) {
        OutputDebugStringA("Failed to create window 2\n");
        return false;
    }

    // Swap chain targets
    m_target1 = std::make_unique<SwapChainTarget>();
    if (!m_target1->Create(*m_device, m_window1->GetHWND(),
                           m_window1->GetWidth(), m_window1->GetHeight())) {
        OutputDebugStringA("Failed to create swap chain target 1\n");
        return false;
    }

    m_target2 = std::make_unique<SwapChainTarget>();
    if (!m_target2->Create(*m_device, m_window2->GetHWND(),
                           m_window2->GetWidth(), m_window2->GetHeight())) {
        OutputDebugStringA("Failed to create swap chain target 2\n");
        return false;
    }

    // Cameras - both windows get independent fly cameras
    m_camera1 = std::make_unique<FlyCamera>();
    float aspect1 = (float)m_window1->GetWidth() / (float)m_window1->GetHeight();
    m_camera1->SetPerspective(m_fovY, aspect1, 0.1f, 100.0f);
    m_camera1->UpdateMatrices();

    m_camera2 = std::make_unique<FlyCamera>();
    float aspect2 = (float)m_window2->GetWidth() / (float)m_window2->GetHeight();
    m_camera2->SetPerspective(m_fovY, aspect2, 0.1f, 100.0f);
    m_camera2->UpdateMatrices();

    // Window 1 renderer (software ray tracer)
    m_renderer = RendererFactory::CreateRenderer(RendererType::Software);
    if (!m_renderer->Initialize(*m_device, m_window1->GetWidth(), m_window1->GetHeight())) {
        OutputDebugStringA("Failed to initialize renderer\n");
        return false;
    }

    // Window 2 renderer (software scene overview)
    m_overviewRenderer = std::make_unique<SoftwareOverviewRenderer>();
    if (!m_overviewRenderer->Initialize(*m_device, m_window2->GetWidth(), m_window2->GetHeight())) {
        OutputDebugStringA("Failed to initialize overview renderer\n");
        return false;
    }

    return true;
}

void Application::SetupScene()
{
	m_scene = std::make_unique<Scene>();

    // Add a sphere
    Material redMat;
    redMat.baseColor = { 1.0f, 1.0f, 0.0f };
    redMat.roughness = 0.1f;
    redMat.ior = 1.5;
    WorldObject* sphereObj = new SphereObject(1.0f, Mat4::Identity(), redMat);
    m_scene->AddObject(sphereObj);

    // Add a light
    Light* light = new PointLight({ 3.0f, 3.0f, -3.0f }, { 1.0f, 1.0f, 1.0f }, 10.0f);
    m_scene->AddLight(light);

    const float PI = 3.14159265f;
    const float boxDist = 4.0f;
    const float quadSize = 8.0f;

    // Floor (Y=-3, normal faces up)
    Material floorMat;
    floorMat.baseColor = { 0.2f, 0.8f, 0.2f }; // green
    floorMat.roughness = 0.0f;
    floorMat.ior = 1.0f;
    m_scene->AddObject(new QuadObject(quadSize, quadSize,
        Mat4::RotationX(-PI * 0.5f) * Mat4::Translation({ 0, -boxDist, 0 }), floorMat));

    // Back wall (Z=+3, normal faces -Z toward camera)
    Material backMat;
    backMat.baseColor = { 0.8f, 0.2f, 0.2f }; // red
    backMat.roughness = 0.0f;
    backMat.ior = 1.0f;
    m_scene->AddObject(new QuadObject(quadSize, quadSize,
        Mat4::RotationY(PI) * Mat4::Translation({ 0, 0, boxDist }), backMat));

    // Left wall (X=-3, normal faces +X)
    Material leftMat;
    leftMat.baseColor = { 0.2f, 0.2f, 0.8f }; // blue
    leftMat.roughness = 0.0f;
    leftMat.ior = 1.0f;
    m_scene->AddObject(new QuadObject(quadSize, quadSize,
        Mat4::RotationY(PI * 0.5f) * Mat4::Translation({ -boxDist, 0, 0 }), leftMat));

    // Right wall (X=+3, normal faces -X)
    Material rightMat;
    rightMat.baseColor = { 0.8f, 0.8f, 0.2f }; // yellow
    rightMat.roughness = 0.0f;
    rightMat.ior = 1.0f;
    m_scene->AddObject(new QuadObject(quadSize, quadSize,
        Mat4::RotationY(-PI * 0.5f) * Mat4::Translation({ boxDist, 0, 0 }), rightMat));

    // Ceiling (Y=+3, normal faces -Y down)
    Material ceilMat;
    ceilMat.baseColor = { 0.8f, 0.2f, 0.8f }; // magenta
    ceilMat.roughness = 0.0f;
    ceilMat.ior = 1.0f;
    m_scene->AddObject(new QuadObject(quadSize, quadSize,
        Mat4::RotationX(PI * 0.5f) * Mat4::Translation({ 0, boxDist, 0 }), ceilMat));
}

void Application::Tick() {
    // Delta time
    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);
    m_deltaTime = (float)(now.QuadPart - m_lastTime.QuadPart) / (float)m_frequency.QuadPart;
    m_lastTime = now;
    if (m_deltaTime > 0.1f) m_deltaTime = 0.1f; // cap to prevent huge jumps

    // Update cameras - each window has its own input and camera
    if (m_input1.IsCaptured()) {
        m_camera1->Update(&m_input1, m_deltaTime);
    }
    m_camera1->ApplyKeyboard(&m_input1, m_deltaTime);
    m_camera1->ApplyMouseWheel(&m_input1);
    m_input1.ResetFrameDeltas();

    if (m_input2.IsCaptured()) {
        m_camera2->Update(&m_input2, m_deltaTime);
    }
    m_camera2->ApplyKeyboard(&m_input2, m_deltaTime);
    m_camera2->ApplyMouseWheel(&m_input2);
    m_input2.ResetFrameDeltas();

    // Handle window 1 resize
    if (m_window1->ConsumeResizeFlag()) {
        uint32_t w = m_window1->GetWidth();
        uint32_t h = m_window1->GetHeight();
        if (w > 0 && h > 0) {
            m_target1->Resize(*m_device, w, h);
            m_renderer->OnResize(*m_device, w, h);
            float aspect = (float)w / (float)h;
            m_camera1->SetPerspective(m_fovY, aspect, 0.1f, 100.0f);
        }
    }

    // Render window 1
    if (m_window1->GetWidth() > 0 && m_window1->GetHeight() > 0) {
        m_renderer->Render(*m_device, *m_scene.get(), *m_camera1, *m_target1);
        m_target1->Present();
    }

    // Handle window 2 resize
    if (m_window2->ConsumeResizeFlag()) {
        uint32_t w = m_window2->GetWidth();
        uint32_t h = m_window2->GetHeight();
        if (w > 0 && h > 0) {
            m_target2->Resize(*m_device, w, h);
            m_overviewRenderer->OnResize(*m_device, w, h);
            float aspect = (float)w / (float)h;
            m_camera2->SetPerspective(m_fovY, aspect, 0.1f, 100.0f);
        }
    }

    // Render window 2 (scene overview showing camera1's frustum)
    if (m_window2->GetWidth() > 0 && m_window2->GetHeight() > 0) {
        m_overviewRenderer->SetObservedCamera(m_camera1.get());
        m_overviewRenderer->Render(*m_device, *m_scene, *m_camera2, *m_target2);
        m_target2->Present();
    }
}

void Application::Shutdown() {
    m_input1.ReleaseCapture();
    m_input2.ReleaseCapture();
    m_overviewRenderer.reset();
    m_renderer.reset();
    m_target2.reset();
    m_target1.reset();
    m_window2.reset();
    m_window1.reset();
    m_device.reset();
}

bool Application::IsRunning() const {
    return m_window1->IsAlive() && m_window2->IsAlive();
}

void Application::OnWindowMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    // Route input to the correct per-window InputManager
    InputManager* input = nullptr;
    if (hwnd == m_window1->GetHWND()) input = &m_input1;
    else if (hwnd == m_window2->GetHWND()) input = &m_input2;
    if (!input) return;

    switch (msg) {
    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE) {
            input->ReleaseCapture();
        } else {
            input->OnKeyDown(wParam);
        }
        break;

    case WM_KEYUP:
        input->OnKeyUp(wParam);
        break;

    case WM_MBUTTONDOWN:
        if (!input->IsCaptured()) {
            input->SetCapture(hwnd, InputManager::Middle);
        }
        break;

    case WM_LBUTTONDOWN:
        if (!input->IsCaptured()) {
            input->SetCapture(hwnd, InputManager::Left);
        }
        break;
    case WM_RBUTTONDOWN:
        if (!input->IsCaptured()) {
            input->SetCapture(hwnd, InputManager::Right);
        }
        break;

    case WM_LBUTTONUP:
    case WM_RBUTTONUP:
    case WM_MBUTTONUP:
		input->ReleaseCapture();
        break;

    case WM_MOUSEMOVE:
        if (input->IsCaptured()) {
            POINT pt;
            GetCursorPos(&pt);
            input->OnRawMouseMove(pt.x, pt.y);
        }
        break;

    case WM_MOUSEWHEEL:
        input->OnMouseWheel(GET_WHEEL_DELTA_WPARAM(wParam));
        break;
    }
}
