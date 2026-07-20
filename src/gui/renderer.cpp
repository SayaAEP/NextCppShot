#include "renderer.h"

#include <imgui.h>
#include <imgui_impl_win32.h>

namespace NextCppShot {

static Renderer* g_current = nullptr;

Renderer* Renderer::GetCurrent() { return g_current; }
void Renderer::SetCurrent(Renderer* r) { g_current = r; }

const wchar_t* RendererTypeName(RendererType type) {
    switch (type) {
        case RendererType::DirectX9:  return L"DirectX 9";
        case RendererType::DirectX10: return L"DirectX 10";
        case RendererType::DirectX11: return L"DirectX 11";
        case RendererType::DirectX12: return L"DirectX 12";
        case RendererType::OpenGL2:   return L"OpenGL 2";
        case RendererType::OpenGL3:   return L"OpenGL 3";
        // case RendererType::Vulkan:    return L"Vulkan";
    }
    return L"Unknown";
}

RendererType RendererTypeFromString(const wchar_t* str) {
    if (!str) return RendererType::DirectX9;
    std::wstring s = str;
    for (auto& c : s) c = towlower(c);
    if (s == L"directx9"  || s == L"dx9"  || s == L"dx" || s == L"directx")   return RendererType::DirectX9;
    if (s == L"directx10" || s == L"dx10")                 return RendererType::DirectX10;
    if (s == L"directx11" || s == L"dx11")                 return RendererType::DirectX11;
    if (s == L"directx12" || s == L"dx12")                 return RendererType::DirectX12;
    if (s == L"opengl2"   || s == L"gl2")                  return RendererType::OpenGL2;
    if (s == L"opengl3"   || s == L"opengl" || s == L"gl3" || s == L"gl")
    return RendererType::OpenGL3;
    // if (s == L"vulkan"    || s == L"vk")                   return RendererType::Vulkan;
    return RendererType::OpenGL3;
}

bool Renderer::InitImGuiContext(HWND hwnd) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    if (!ImGui_ImplWin32_Init(hwnd)) return false;

    ImGui::StyleColorsDark();
    ImVec4* colors = ImGui::GetStyle().Colors;
    colors[ImGuiCol_WindowBg]      = ImVec4(0.12f, 0.12f, 0.12f, 1.0f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.2f,  0.4f,  0.8f,  1.0f);
    colors[ImGuiCol_Button]        = ImVec4(0.2f,  0.4f,  0.8f,  1.0f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.3f,  0.5f,  0.9f,  1.0f);
    colors[ImGuiCol_ButtonActive]  = ImVec4(0.1f,  0.3f,  0.7f,  1.0f);
    return true;
}

void Renderer::ShutdownImGuiContext() {
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
}

Renderer* CreateRenderer(RendererType type) {
    switch (type) {
        case RendererType::DirectX9:  return CreateDX9Backend();
        case RendererType::DirectX10: return CreateDX10Backend();
        case RendererType::DirectX11: return CreateDX11Backend();
        case RendererType::DirectX12: return CreateDX12Backend();
        case RendererType::OpenGL2:   return CreateOpenGL2Backend();
        case RendererType::OpenGL3:   return CreateOpenGL3Backend();
        // case RendererType::Vulkan:    return CreateVulkanBackend();
    }
    return nullptr;
}

} // namespace NextCppShot