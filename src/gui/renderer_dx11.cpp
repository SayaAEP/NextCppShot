#include "renderer.h"

namespace NextCppShot {

class RendererDX11 : public Renderer {
public:
    bool Init(HWND hwnd) override {
        (void)hwnd;
        MessageBoxW(NULL, L"DirectX 11 renderer is not yet implemented.",
                    L"NextCppShot", MB_OK | MB_ICONWARNING);
        return false;
    }
    void Shutdown() override {}
    void NewFrame() override {}
    void RenderDrawData(ImDrawData* data) override { (void)data; }
    void Clear() override {}
    void Present() override {}
    void OnResize(int, int) override {}
    RendererType GetType() const override { return RendererType::DirectX11; }
};

Renderer* CreateDX11Backend() { return new RendererDX11(); }

} // namespace NextCppShot