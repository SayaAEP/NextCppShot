#include "Gui.h"
#include "renderer.h"

#include <imgui.h>
#include <imgui_impl_win32.h>

#include <shellapi.h>

// ImGui's WndProcHandler is defined in the global namespace by the Win32 backend.
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace NextCppShot {
namespace Gui {

// =============================================================================
// Globals - Window
// =============================================================================
HWND g_hwnd = nullptr;
HINSTANCE g_hInstance = nullptr;

// =============================================================================
// Globals - State
// =============================================================================
static bool g_wantsScreenshot = false;
static bool g_wantsCRE = false;
static bool g_includeInactive = true;
static bool g_autoOpenFolder = true;
static std::wstring g_lastFilename;
static size_t g_lastFileSize = 0;

// Drag and resize state
static bool g_isDragging = false;
static bool g_isResizing = false;
static int g_resizeDirection = 0;
static POINTS g_dragStart = {};
static POINT g_resizeStart = {};
static RECT g_windowStartRect = {};

// =============================================================================
// Constants
// =============================================================================
static const int TITLEBAR_HEIGHT = 32;
static const int RESIZE_BORDER = 8;
static const float CLOSE_BTN_WIDTH = 46.0f;
static const int DEFAULT_WIDTH = 520;
static const int DEFAULT_HEIGHT = 480;
static const int MIN_WIDTH = 400;
static const int MIN_HEIGHT = 300;

// Resize direction enum
enum ResizeDirection {
    RESIZE_NONE = 0,
    RESIZE_LEFT = 1,
    RESIZE_RIGHT = 2,
    RESIZE_TOP = 4,
    RESIZE_BOTTOM = 8,
    RESIZE_TOPLEFT = RESIZE_TOP | RESIZE_LEFT,
    RESIZE_TOPRIGHT = RESIZE_TOP | RESIZE_RIGHT,
    RESIZE_BOTTOMLEFT = RESIZE_BOTTOM | RESIZE_LEFT,
    RESIZE_BOTTOMRIGHT = RESIZE_BOTTOM | RESIZE_RIGHT,
};

// =============================================================================
// Helper Functions
// =============================================================================
static bool IsImGuiContextReady() {
    return ImGui::GetCurrentContext() != nullptr;
}

static void UpdateDisplaySize(int width, int height) {
    if (IsImGuiContextReady()) {
        ImGuiIO& io = ImGui::GetIO();
        io.DisplaySize = ImVec2((float)width, (float)height);
    }
}

// =============================================================================
// Window Procedure — 100% renderer-agnostic
// =============================================================================
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    // Let ImGui handle input only when ImGui is initialized.
    if (IsImGuiContextReady() && ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam))
        return true;

    switch (msg) {
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU)
            return 0;
        break;

    case WM_LBUTTONDOWN: {
        POINTS pts = MAKEPOINTS(lParam);
        float displayW = 0.0f, displayH = 0.0f;
        if (IsImGuiContextReady()) {
            ImGuiIO& io = ImGui::GetIO();
            displayW = io.DisplaySize.x;
            displayH = io.DisplaySize.y;
        }

        int resizeDir = 0;
        bool onRight = pts.x >= (int)(displayW - RESIZE_BORDER);
        bool onLeft = pts.x <= RESIZE_BORDER - 1;
        bool onBottom = pts.y >= (int)(displayH - RESIZE_BORDER);
        bool onTop = pts.y <= RESIZE_BORDER - 1;

        if (onRight) resizeDir |= RESIZE_RIGHT;
        if (onLeft) resizeDir |= RESIZE_LEFT;
        if (onBottom) resizeDir |= RESIZE_BOTTOM;
        if (onTop) resizeDir |= RESIZE_TOP;

        if (resizeDir != 0) {
            g_isResizing = true;
            g_resizeDirection = resizeDir;
            GetWindowRect(hwnd, &g_windowStartRect);
            GetCursorPos(&g_resizeStart);
            SetCapture(hwnd);
            g_dragStart = pts;
        }
        else if (pts.y <= TITLEBAR_HEIGHT && pts.x < (int)(displayW - CLOSE_BTN_WIDTH * 2 - 2)) {
            g_isDragging = true;
            g_dragStart = pts;
            SetCapture(hwnd);
        }
        else {
            ReleaseCapture();
        }
        return 0;
    }

    case WM_LBUTTONUP: {
        g_isDragging = false;
        g_isResizing = false;
        ReleaseCapture();
        return 0;
    }

    case WM_MOUSEMOVE: {
        POINTS pts = MAKEPOINTS(lParam);
        float displayW = 0.0f, displayH = 0.0f;
        if (IsImGuiContextReady()) {
            ImGuiIO& io = ImGui::GetIO();
            displayW = io.DisplaySize.x;
            displayH = io.DisplaySize.y;
        }

        int resizeDir = 0;
        bool onRight = pts.x >= (int)(displayW - RESIZE_BORDER);
        bool onLeft = pts.x <= RESIZE_BORDER - 1;
        bool onBottom = pts.y >= (int)(displayH - RESIZE_BORDER);
        bool onTop = pts.y <= RESIZE_BORDER - 1;

        if (onRight) resizeDir |= RESIZE_RIGHT;
        if (onLeft) resizeDir |= RESIZE_LEFT;
        if (onBottom) resizeDir |= RESIZE_BOTTOM;
        if (onTop) resizeDir |= RESIZE_TOP;

        // Handle resize
        if (g_isResizing) {
            POINT pt;
            GetCursorPos(&pt);
            int dx = pt.x - g_resizeStart.x;
            int dy = pt.y - g_resizeStart.y;

            RECT newRect = g_windowStartRect;
            LONG left = newRect.left;
            LONG top = newRect.top;
            LONG right = newRect.right;
            LONG bottom = newRect.bottom;

            if (g_resizeDirection & RESIZE_LEFT) left += dx;
            if (g_resizeDirection & RESIZE_RIGHT) right += dx;
            if (g_resizeDirection & RESIZE_TOP) top += dy;
            if (g_resizeDirection & RESIZE_BOTTOM) bottom += dy;

            int newWidth = (int)(right - left);
            int newHeight = (int)(bottom - top);

            if (newWidth >= MIN_WIDTH && newHeight >= MIN_HEIGHT) {
                SetWindowPos(hwnd, NULL, (int)left, (int)top, newWidth, newHeight, SWP_NOZORDER);
                UpdateDisplaySize(newWidth, newHeight);
                if (auto* r = Renderer::GetCurrent()) r->OnResize(newWidth, newHeight);
            }
        }

        // Handle drag
        if (g_isDragging && !g_isResizing) {
            RECT winRect;
            GetWindowRect(hwnd, &winRect);
            int newX = winRect.left + (pts.x - g_dragStart.x);
            int newY = winRect.top + (pts.y - g_dragStart.y);
            SetWindowPos(hwnd, HWND_TOP, newX, newY, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
        }

        // Set cursor
        switch (resizeDir) {
        case RESIZE_RIGHT:
        case RESIZE_LEFT:
            SetCursor(LoadCursor(NULL, IDC_SIZEWE));
            break;
        case RESIZE_TOP:
        case RESIZE_BOTTOM:
            SetCursor(LoadCursor(NULL, IDC_SIZENS));
            break;
        case RESIZE_TOPLEFT:
        case RESIZE_BOTTOMRIGHT:
            SetCursor(LoadCursor(NULL, IDC_SIZENWSE));
            break;
        case RESIZE_TOPRIGHT:
        case RESIZE_BOTTOMLEFT:
            SetCursor(LoadCursor(NULL, IDC_SIZENESW));
            break;
        default:
            if (pts.y <= TITLEBAR_HEIGHT && pts.x < (int)(displayW - CLOSE_BTN_WIDTH * 2 - 2))
                SetCursor(LoadCursor(NULL, IDC_SIZEALL));
            else
                SetCursor(LoadCursor(NULL, IDC_ARROW));
            break;
        }
        return 0;
    }

    case WM_ACTIVATE:
        if (wParam != WA_INACTIVE)
            ForceRender();
        return 0;

    case WM_SIZE: {
        if (wParam != SIZE_MINIMIZED) {
            UINT width = LOWORD(lParam);
            UINT height = HIWORD(lParam);
            UpdateDisplaySize(width, height);
            if (auto* r = Renderer::GetCurrent()) r->OnResize((int)width, (int)height);
            ForceRender();
        }
        return 0;
    }

    case WM_WINDOWPOSCHANGED:
        ForceRender();
        return 0;

    case WM_EXITSIZEMOVE: {
        ForceRender();
        return 0;
    }

    case WM_PAINT:
        ValidateRect(hwnd, NULL);
        return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

// =============================================================================
// Window Init
// =============================================================================
void InitWindow(HINSTANCE hInstance, int nCmdShow) {
    g_hInstance = hInstance;

    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.style = CS_CLASSDC;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = L"NextCppShot";

    if (!RegisterClassExW(&wc)) {
        MessageBox(NULL, L"Failed to register window class", L"NextCppShot Error", MB_ICONERROR);
        return;
    }

    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);
    int posX = (screenW - DEFAULT_WIDTH) / 2;
    int posY = (screenH - DEFAULT_HEIGHT) / 3;

    g_hwnd = CreateWindowW(wc.lpszClassName, L"NextCppShot",
        WS_POPUP | WS_SYSMENU | WS_MINIMIZEBOX,
        posX, posY, DEFAULT_WIDTH, DEFAULT_HEIGHT,
        NULL, NULL, hInstance, NULL);

    if (!g_hwnd) {
        MessageBox(NULL, L"Failed to create window", L"NextCppShot Error", MB_ICONERROR);
        return;
    }

    ShowWindow(g_hwnd, nCmdShow);
    UpdateWindow(g_hwnd);
}

void ShutdownWindow() {
    // Nothing to clean up
}

// =============================================================================
// Gui Init/Shutdown — delegated to Renderer
// =============================================================================
bool Init() {
    Renderer* r = Renderer::GetCurrent();
    if (!r) return false;
    if (!r->Init(g_hwnd)) return false;
    return true;
}

void Shutdown() {
    if (auto* r = Renderer::GetCurrent()) r->Shutdown();
}

// =============================================================================
// UI Drawing — renderer-agnostic ImGui widgets
// =============================================================================
static void DrawUI() {
    ImGuiIO& io = ImGui::GetIO();
    float titlebarH = (float)TITLEBAR_HEIGHT;
    float btnW = CLOSE_BTN_WIDTH;
    float statusbarH = 28.0f;

    // Main window
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2(io.DisplaySize.x, io.DisplaySize.y));

    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoTitleBar |
                                    ImGuiWindowFlags_NoResize |
                                    ImGuiWindowFlags_NoMove |
                                    ImGuiWindowFlags_NoCollapse |
                                    ImGuiWindowFlags_NoBringToFrontOnFocus |
                                    ImGuiWindowFlags_NoNavFocus |
                                    ImGuiWindowFlags_NoScrollbar;

    ImGui::Begin("##main", nullptr, window_flags);

    // TITLE BAR
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImVec2 winPos = ImGui::GetWindowPos();

    drawList->AddRectFilled(
        ImVec2(winPos.x, winPos.y),
        ImVec2(winPos.x + io.DisplaySize.x, winPos.y + titlebarH),
        IM_COL32(25, 45, 85, 255));

    // Draggable area
    ImGui::SetCursorPos(ImVec2(0, 0));
    ImGui::InvisibleButton("##titlebar_drag", ImVec2(io.DisplaySize.x - btnW * 2 - 2, titlebarH));
    if (ImGui::IsItemHovered())
        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);

    // Title text
    ImGui::SetCursorPos(ImVec2(10, (titlebarH - 16) * 0.5f));
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
    ImGui::Text("NextCppShot");
    ImGui::PopStyleColor();

    // Close button
    ImGui::SetCursorPos(ImVec2(io.DisplaySize.x - btnW, 0));
    ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(200, 50, 50, 255));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(255, 80, 80, 255));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(150, 30, 30, 255));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.0f);
    if (ImGui::Button("X", ImVec2(btnW, titlebarH))) {
        PostQuitMessage(0);
    }
    ImGui::PopStyleColor(3);
    ImGui::PopStyleVar();

    // Minimize button
    ImGui::SetCursorPos(ImVec2(io.DisplaySize.x - btnW * 2 - 2, 0));
    ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(60, 60, 60, 255));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(80, 80, 80, 255));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(40, 40, 40, 255));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.0f);
    if (ImGui::Button("-", ImVec2(btnW, titlebarH))) {
        ShowWindow(g_hwnd, SW_MINIMIZE);
    }
    ImGui::PopStyleColor(3);
    ImGui::PopStyleVar();

    // CONTENT AREA
    float contentTop = titlebarH + 4;
    float contentBottom = io.DisplaySize.y - statusbarH - 4;
    float contentHeight = contentBottom - contentTop;

    ImGui::SetCursorPosY(contentTop);
    ImGui::BeginChild("##content", ImVec2(0, contentHeight), false, ImGuiWindowFlags_NoMove);

    ImGui::Indent(8);

    // Renderer info
    const wchar_t* rendererName = RendererTypeName(Renderer::GetCurrent() ? Renderer::GetCurrent()->GetType() : RendererType::DirectX9);
    char rendererNameUtf8[64];
    WideCharToMultiByte(CP_UTF8, 0, rendererName, -1, rendererNameUtf8, sizeof(rendererNameUtf8), NULL, NULL);

    ImGui::TextColored(ImVec4(0.4f, 0.7f, 1.0f, 1.0f), "NextCppShot");
    ImGui::SameLine();
    ImGui::TextDisabled("(Fork of CppShot)");
    ImGui::TextDisabled("Renderer: %s", rendererNameUtf8);
    ImGui::TextWrapped("Transparent Screenshot Utility");
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::Text("Status: ");
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(0.4f, 0.9f, 0.4f, 1.0f), "Ready");
    ImGui::Spacing();

    float btnCaptureW = 300.0f;
    ImGui::SetCursorPosX((ImGui::GetWindowWidth() - btnCaptureW) * 0.5f);
    if (ImGui::Button("Take Screenshot (Ctrl+B)", ImVec2(btnCaptureW, 50)))
        g_wantsScreenshot = true;

    ImGui::Spacing();

    ImGui::SetCursorPosX((ImGui::GetWindowWidth() - btnCaptureW) * 0.5f);
    if (ImGui::Button("Take CRE Screenshot (Ctrl+Shift+B)", ImVec2(btnCaptureW, 50)))
        g_wantsCRE = true;

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Options");
    ImGui::Spacing();
    ImGui::TextDisabled("[ ] Include inactive window (CRE)");
    ImGui::TextDisabled("[ ] Auto-open folder after capture");
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    if (!g_lastFilename.empty()) {
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Last Capture:");
        ImGui::TextWrapped("%ls", g_lastFilename.c_str());
        ImGui::Text("Size: %.2f MB", g_lastFileSize / (1024.0 * 1024.0));
        ImGui::Spacing();
    }

    ImGui::SetCursorPosX((ImGui::GetWindowWidth() - 200) * 0.5f);
    if (ImGui::Button("Open Screenshots Folder", ImVec2(200, 35)))
        ShellExecuteW(NULL, L"open", GetSaveDirectoryPath().c_str(), NULL, NULL, SW_SHOWNORMAL);

    ImGui::Unindent(8);
    ImGui::EndChild();

    // STATUS BAR
    ImGui::SetCursorPosY(io.DisplaySize.y - statusbarH);
    ImGui::Separator();
    ImGui::SetCursorPosX(8);
    ImGui::TextDisabled("NextCppShot v0.7.0 | Transparent Screenshot Utility");

    ImGui::End();

    // RESIZE GRIPPERS
    ImDrawList* gripperList = ImGui::GetWindowDrawList();
    ImVec2 gripWinPos = ImGui::GetWindowPos();
    ImVec2 gripSize = io.DisplaySize;
    ImU32 gripColor = IM_COL32(80, 80, 80, 200);

    gripperList->AddLine(
        ImVec2(gripWinPos.x + 16, gripWinPos.y + gripSize.y - 4),
        ImVec2(gripWinPos.x + gripSize.x - 16, gripWinPos.y + gripSize.y - 4),
        gripColor, 2.0f);

    gripperList->AddLine(
        ImVec2(gripWinPos.x + gripSize.x - 4, gripWinPos.y + 16),
        ImVec2(gripWinPos.x + gripSize.x - 4, gripWinPos.y + gripSize.y - 16),
        gripColor, 2.0f);

    float gripX = gripWinPos.x + gripSize.x - 8;
    float gripY = gripWinPos.y + gripSize.y - 8;
    gripperList->AddCircleFilled(ImVec2(gripX, gripY), 4.0f, gripColor);
    gripperList->AddCircleFilled(ImVec2(gripX - 8, gripY), 3.0f, gripColor);
    gripperList->AddCircleFilled(ImVec2(gripX, gripY - 8), 3.0f, gripColor);

    ImGui::EndFrame();
    ImGui::Render();
}

// =============================================================================
// ForceRender — single point of renderer dispatch
// =============================================================================
void ForceRender() {
    Renderer* r = Renderer::GetCurrent();
    if (!r || !g_hwnd || IsIconic(g_hwnd)) return;
    // Don't render if ImGui context hasn't been initialized yet
    // (WM_SIZE fires during CreateWindowEx before Gui::Init)
    if (!IsImGuiContextReady()) return;

    r->Clear();
    r->NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
    DrawUI();
    r->RenderDrawData(ImGui::GetDrawData());
    r->Present();
}

// =============================================================================
// State
// =============================================================================
void SetWantsScreenshot(bool v) { g_wantsScreenshot = v; }
bool WantsScreenshot() { return g_wantsScreenshot; }

void SetWantsCRE(bool v) { g_wantsCRE = v; }
bool WantsCRE() { return g_wantsCRE; }

void SetLastCapture(const std::wstring& filename, size_t size) {
    g_lastFilename = filename;
    g_lastFileSize = size;
}
std::wstring GetLastCaptureFilename() { return g_lastFilename; }
size_t GetLastCaptureSize() { return g_lastFileSize; }

bool GetIncludeInactive() { return g_includeInactive; }
void SetIncludeInactive(bool v) { g_includeInactive = v; }

bool GetAutoOpenFolder() { return g_autoOpenFolder; }
void SetAutoOpenFolder(bool v) { g_autoOpenFolder = v; }

std::wstring GetSaveDirectoryPath() {
    HKEY hKey = NULL;
    if (RegOpenKeyW(HKEY_CURRENT_USER, L"SOFTWARE\\CppShot", &hKey) == ERROR_SUCCESS) {
        wchar_t szValue[1024];
        DWORD cbValueLength = sizeof(szValue);
        if (RegQueryValueExW(hKey, L"Path", NULL, NULL, (LPBYTE)&szValue, &cbValueLength) == ERROR_SUCCESS) {
            RegCloseKey(hKey);
            return std::wstring(szValue);
        }
        RegCloseKey(hKey);
    }
    return std::wstring(L"C:\\test\\");
}

} // namespace Gui
} // namespace NextCppShot