#include "renderer.h"

namespace NextCppShot {

class RendererOpenGL2 : public Renderer {
public:
    bool Init(HWND hwnd) override {
        (void)hwnd;
        MessageBoxW(NULL, L"OpenGL 2 renderer is not yet implemented.",
                    L"NextCppShot", MB_OK | MB_ICONWARNING);
        return false;
    }
    void Shutdown() override {}
    void NewFrame() override {}
    void RenderDrawData(ImDrawData* data) override { (void)data; }
    void Clear() override {}
    void Present() override {}
    void OnResize(int, int) override {}
    RendererType GetType() const override { return RendererType::OpenGL2; }
};

Renderer* CreateOpenGL2Backend() { return new RendererOpenGL2(); }

} // namespace NextCppShot