#include "renderer.h"

#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include <GL/gl.h>
#include <GL/glu.h>

namespace NextCppShot {

class RendererOpenGL3 : public Renderer {
    HWND  m_hwnd = nullptr;
    HDC   m_hDC  = nullptr;
    HGLRC m_hRC  = nullptr;

public:
    bool Init(HWND hwnd) override {
        m_hwnd = hwnd;
        m_hDC = GetDC(hwnd);
        if (!m_hDC) {
            MessageBox(hwnd, L"GetDC() failed!", L"NextCppShot Error", MB_OK | MB_ICONERROR);
            return false;
        }

        PIXELFORMATDESCRIPTOR pfd = {};
        pfd.nSize         = sizeof(PIXELFORMATDESCRIPTOR);
        pfd.nVersion      = 1;
        pfd.dwFlags       = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL
                          | PFD_SUPPORT_COMPOSITION | PFD_DOUBLEBUFFER;
        pfd.iPixelType    = PFD_TYPE_RGBA;
        pfd.cColorBits    = 32;
        pfd.cAlphaBits    = 8;
        pfd.cDepthBits    = 24;
        pfd.cStencilBits  = 8;
        pfd.iLayerType    = PFD_MAIN_PLANE;

        int pf = ChoosePixelFormat(m_hDC, &pfd);
        if (!pf) {
            MessageBox(hwnd, L"ChoosePixelFormat() failed!", L"NextCppShot Error", MB_OK | MB_ICONERROR);
            return false;
        }
        if (!SetPixelFormat(m_hDC, pf, &pfd)) {
            MessageBox(hwnd, L"SetPixelFormat() failed!", L"NextCppShot Error", MB_OK | MB_ICONERROR);
            return false;
        }

        m_hRC = wglCreateContext(m_hDC);
        if (!m_hRC) {
            MessageBox(hwnd, L"wglCreateContext() failed!", L"NextCppShot Error", MB_OK | MB_ICONERROR);
            return false;
        }
        if (!wglMakeCurrent(m_hDC, m_hRC)) {
            MessageBox(hwnd, L"wglMakeCurrent() failed!", L"NextCppShot Error", MB_OK | MB_ICONERROR);
            return false;
        }

        typedef BOOL(WINAPI * PFNWGLSWAPINTERVALEXTPROC)(int);
        if (auto wglSwapIntervalEXT = (PFNWGLSWAPINTERVALEXTPROC)wglGetProcAddress("wglSwapIntervalEXT"))
            wglSwapIntervalEXT(1);

        if (!InitImGuiContext(hwnd)) return false;
        if (!ImGui_ImplOpenGL3_Init("#version 130")) return false;
        return true;
    }

    void Shutdown() override {
        ImGui_ImplOpenGL3_Shutdown();
        ShutdownImGuiContext();
        if (m_hRC)        { wglMakeCurrent(NULL, NULL); wglDeleteContext(m_hRC); m_hRC = nullptr; }
        if (m_hDC && m_hwnd) { ReleaseDC(m_hwnd, m_hDC); m_hDC = nullptr; }
    }

    void NewFrame() override { ImGui_ImplOpenGL3_NewFrame(); }

    void RenderDrawData(ImDrawData* draw_data) override {
        ImGui_ImplOpenGL3_RenderDrawData(draw_data);
    }

    void Clear() override {
        glClearColor(0.12f, 0.12f, 0.12f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    void Present() override {
        if (m_hDC) SwapBuffers(m_hDC);
    }

    void OnResize(int, int) override { /* OpenGL viewport auto-resizes */ }

    RendererType GetType() const override { return RendererType::OpenGL3; }
};

Renderer* CreateOpenGL3Backend() { return new RendererOpenGL3(); }

} // namespace NextCppShot