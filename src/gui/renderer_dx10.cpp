#include "renderer.h"

#include <imgui.h>
#include <imgui_impl_dx10.h>
#include <d3d10_1.h>
#include <dxgi.h>

#pragma comment(lib, "d3d10.lib")
#pragma comment(lib, "dxgi.lib")

namespace NextCppShot {

class RendererDX10 : public Renderer {
    HWND                m_hwnd   = nullptr;
    IDXGIFactory*       m_pFactory = nullptr;
    ID3D10Device*       m_pd3dDevice = nullptr;
    IDXGISwapChain*     m_pSwapChain = nullptr;
    ID3D10RenderTargetView* m_pMainRenderTargetView = nullptr;
    DXGI_SWAP_CHAIN_DESC m_sd = {};

    bool createRenderTarget() {
        if (m_pSwapChain) {
            if (m_pMainRenderTargetView) {
                m_pMainRenderTargetView->Release();
                m_pMainRenderTargetView = nullptr;
            }
            ID3D10Texture2D* pBackBuffer;
            if (SUCCEEDED(m_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer)))) {
                m_pd3dDevice->CreateRenderTargetView(pBackBuffer, NULL, &m_pMainRenderTargetView);
                pBackBuffer->Release();
            }
        }
        return m_pMainRenderTargetView != nullptr;
    }

    bool createDeviceAndSwapChain(HWND hwnd) {
        RECT rc;
        GetClientRect(hwnd, &rc);
        UINT width = rc.right - rc.left;
        UINT height = rc.bottom - rc.top;

        m_sd.BufferCount = 2;
        m_sd.BufferDesc.Width = width;
        m_sd.BufferDesc.Height = height;
        m_sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        m_sd.BufferDesc.RefreshRate.Numerator = 60;
        m_sd.BufferDesc.RefreshRate.Denominator = 1;
        m_sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        m_sd.OutputWindow = hwnd;
        m_sd.SampleDesc.Count = 1;
        m_sd.SampleDesc.Quality = 0;
        m_sd.Windowed = TRUE;
        m_sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

        UINT deviceFlags = D3D10_CREATE_DEVICE_BGRA_SUPPORT;
#ifdef _DEBUG
        deviceFlags |= D3D10_CREATE_DEVICE_DEBUG;
#endif

        IDXGIAdapter* pAdapter = nullptr;
        IDXGIOutput* pOutput = nullptr;
        if (m_pFactory) {
            if (SUCCEEDED(m_pFactory->EnumAdapters(0, &pAdapter))) {
                pAdapter->EnumOutputs(0, &pOutput);
            }
        }

        HRESULT hr = D3D10CreateDeviceAndSwapChain(
            pAdapter, D3D10_DRIVER_TYPE_HARDWARE,
            NULL, deviceFlags, D3D10_SDK_VERSION,
            &m_sd, &m_pSwapChain, &m_pd3dDevice);

        if (pOutput) pOutput->Release();
        if (pAdapter) pAdapter->Release();

        if (FAILED(hr)) {
            return false;
        }

        return createRenderTarget();
    }

public:
    bool Init(HWND hwnd) override {
        m_hwnd = hwnd;

        if (FAILED(CreateDXGIFactory(IID_PPV_ARGS(&m_pFactory)))) {
            MessageBox(hwnd, L"CreateDXGIFactory() failed!", L"NextCppShot Error", MB_OK | MB_ICONERROR);
            return false;
        }

        if (!createDeviceAndSwapChain(hwnd)) {
            MessageBox(hwnd, L"Failed to create DX10 device and swap chain!", L"NextCppShot Error", MB_OK | MB_ICONERROR);
            return false;
        }

        if (!InitImGuiContext(hwnd)) return false;
        if (!ImGui_ImplDX10_Init(m_pd3dDevice)) return false;

        return true;
    }

    void Shutdown() override {
        ImGui_ImplDX10_Shutdown();
        ShutdownImGuiContext();
        if (m_pMainRenderTargetView) { m_pMainRenderTargetView->Release(); m_pMainRenderTargetView = nullptr; }
        if (m_pSwapChain) { m_pSwapChain->Release(); m_pSwapChain = nullptr; }
        if (m_pd3dDevice) { m_pd3dDevice->Release(); m_pd3dDevice = nullptr; }
        if (m_pFactory) { m_pFactory->Release(); m_pFactory = nullptr; }
    }

    void NewFrame() override { ImGui_ImplDX10_NewFrame(); }

    void RenderDrawData(ImDrawData* draw_data) override {
        if (!m_pd3dDevice || !m_pMainRenderTargetView) return;
        m_pd3dDevice->OMSetRenderTargets(1, &m_pMainRenderTargetView, NULL);
        ImGui_ImplDX10_RenderDrawData(draw_data);
    }

    void Clear() override {
        if (!m_pd3dDevice || !m_pMainRenderTargetView) return;
        m_pd3dDevice->OMSetRenderTargets(1, &m_pMainRenderTargetView, NULL);
        float clearColor[] = { 0.12f, 0.12f, 0.12f, 1.0f };
        m_pd3dDevice->ClearRenderTargetView(m_pMainRenderTargetView, clearColor);
    }

    void Present() override {
        if (!m_pSwapChain) return;
        m_pSwapChain->Present(0, 0);
    }

    void OnResize(int width, int height) override {
        if (!m_pSwapChain || !m_pd3dDevice || width <= 0 || height <= 0) return;

        ImGui_ImplDX10_InvalidateDeviceObjects();

        if (m_pMainRenderTargetView) {
            m_pMainRenderTargetView->Release();
            m_pMainRenderTargetView = nullptr;
        }

        HRESULT hr = m_pSwapChain->ResizeBuffers(m_sd.BufferCount, width, height,
                                                  m_sd.BufferDesc.Format, m_sd.Flags);
        if (FAILED(hr)) return;

        m_pd3dDevice->OMSetRenderTargets(0, NULL, NULL);
        createRenderTarget();
        ImGui_ImplDX10_CreateDeviceObjects();
    }

    RendererType GetType() const override { return RendererType::DirectX10; }
};

Renderer* CreateDX10Backend() { return new RendererDX10(); }

} // namespace NextCppShot
