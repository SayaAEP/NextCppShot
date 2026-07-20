#include "renderer.h"

namespace NextCppShot {

class RendererVulkan : public Renderer {
public:
    bool Init(HWND hwnd) override {
        (void)hwnd;
        MessageBoxW(NULL, L"Vulkan renderer is not yet implemented.",
                    L"NextCppShot", MB_OK | MB_ICONWARNING);
        return false;
    }
    void Shutdown() override {}
    void NewFrame() override {}
    void RenderDrawData(ImDrawData* data) override { (void)data; }
    void Clear() override {}
    void Present() override {}
    void OnResize(int, int) override {}
    RendererType GetType() const override { return RendererType::Vulkan; }
};

Renderer* CreateVulkanBackend() { return new RendererVulkan(); }

} // namespace NextCppShot