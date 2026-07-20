#include "renderer.h"

#include <imgui.h>
#include <imgui_impl_dx9.h>
#include <d3d9.h>

namespace NextCppShot {

class RendererDX9 : public Renderer {
    LPDIRECT3D9            m_pD3D       = nullptr;
    LPDIRECT3DDEVICE9      m_pd3dDevice = nullptr;
    D3DPRESENT_PARAMETERS  m_d3dpp      = {};
    HWND                   m_hwnd       = nullptr;

public:
    bool Init(HWND hwnd) override {
        m_hwnd = hwnd;
        if (NULL == (m_pD3D = Direct3DCreate9(D3D_SDK_VERSION))) {
            MessageBox(hwnd, L"Direct3DCreate9() failed!", L"NextCppShot Error", MB_OK | MB_ICONERROR);
            return false;
        }

        ZeroMemory(&m_d3dpp, sizeof(m_d3dpp));
        m_d3dpp.Windowed               = TRUE;
        m_d3dpp.SwapEffect             = D3DSWAPEFFECT_DISCARD;
        m_d3dpp.BackBufferFormat       = D3DFMT_UNKNOWN;
        m_d3dpp.EnableAutoDepthStencil = TRUE;
        m_d3dpp.AutoDepthStencilFormat = D3DFMT_D16;
        m_d3dpp.PresentationInterval   = D3DPRESENT_INTERVAL_IMMEDIATE;

        if (m_pD3D->CreateDevice(
                D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hwnd,
                D3DCREATE_HARDWARE_VERTEXPROCESSING | D3DCREATE_MULTITHREADED,
                &m_d3dpp, &m_pd3dDevice) != D3D_OK) {
            MessageBox(hwnd, L"CreateDevice() failed!", L"NextCppShot Error", MB_OK | MB_ICONERROR);
            return false;
        }

        if (!InitImGuiContext(hwnd)) return false;
        if (!ImGui_ImplDX9_Init(m_pd3dDevice)) return false;
        return true;
    }

    void Shutdown() override {
        ImGui_ImplDX9_Shutdown();
        ShutdownImGuiContext();
        if (m_pd3dDevice) { m_pd3dDevice->Release(); m_pd3dDevice = nullptr; }
        if (m_pD3D)       { m_pD3D->Release();       m_pD3D       = nullptr; }
    }

    void NewFrame() override { ImGui_ImplDX9_NewFrame(); }

    void RenderDrawData(ImDrawData* draw_data) override {
        ImGui_ImplDX9_RenderDrawData(draw_data);
    }

    void Clear() override {
        if (!m_pd3dDevice) return;
        D3DCOLOR clearCol = D3DCOLOR_RGBA(30, 30, 30, 255);
        m_pd3dDevice->Clear(0, nullptr, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, clearCol, 1.0f, 0);
        m_pd3dDevice->BeginScene();
    }

    void Present() override {
        if (!m_pd3dDevice) return;
        m_pd3dDevice->EndScene();
        m_pd3dDevice->Present(NULL, NULL, NULL, NULL);
    }

    void OnResize(int width, int height) override {
        if (!m_pd3dDevice || !m_hwnd) return;
        m_d3dpp.BackBufferWidth  = width;
        m_d3dpp.BackBufferHeight = height;
        m_pd3dDevice->Reset(&m_d3dpp);
    }

    RendererType GetType() const override { return RendererType::DirectX9; }
};

Renderer* CreateDX9Backend() { return new RendererDX9(); }

} // namespace NextCppShot