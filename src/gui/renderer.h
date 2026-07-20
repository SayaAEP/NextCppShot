#pragma once

#include <windows.h>
#include <string>

// Forward decl — keep ImGui out of header
struct ImDrawData;

namespace NextCppShot {

enum class RendererType {
    DirectX9,
    DirectX10,
    DirectX11,
    DirectX12,
    OpenGL2,
    OpenGL3
    // Vulkan   // Enable when Vulkan SDK is installed
};

const wchar_t* RendererTypeName(RendererType type);
RendererType   RendererTypeFromString(const wchar_t* str);

class Renderer {
public:
    virtual ~Renderer() = default;

    // Lifecycle
    virtual bool Init(HWND hwnd) = 0;
    virtual void Shutdown() = 0;

    // Per-frame
    virtual void NewFrame() = 0;
    virtual void RenderDrawData(ImDrawData* draw_data) = 0;

    // Clear + present split (DX9 needs BeginScene/EndScene, GL3 doesn't)
    virtual void Clear() = 0;
    virtual void Present() = 0;

    // For DX9 device-reset on resize; no-op for GL3
    virtual void OnResize(int width, int height) = 0;

    // For UI display
    virtual RendererType GetType() const = 0;

    // Singleton accessor — used by WndProc and main loop
    static Renderer* GetCurrent();
    static void      SetCurrent(Renderer* r);

protected:
    // Shared ImGui Win32 bootstrap — defined in renderer.cpp
    bool InitImGuiContext(HWND hwnd);
    void ShutdownImGuiContext();
};

// Factory — defined in renderer.cpp
Renderer* CreateRenderer(RendererType type);

// Backend entry points — each renderer_*.cpp defines one
Renderer* CreateDX9Backend();
Renderer* CreateDX10Backend();
Renderer* CreateDX11Backend();
Renderer* CreateDX12Backend();
Renderer* CreateOpenGL2Backend();
Renderer* CreateOpenGL3Backend();
// Renderer* CreateVulkanBackend();   // Enable when Vulkan SDK is installed

} // namespace NextCppShot
