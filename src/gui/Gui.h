#pragma once

#include <windows.h>
#include <string>

namespace NextCppShot {
namespace Gui {

// =============================================================================
// Window
// =============================================================================
extern HWND g_hwnd;

void InitWindow(HINSTANCE hInstance, int nCmdShow);
void ShutdownWindow();

// =============================================================================
// Lifecycle
// =============================================================================
bool Init();
void Shutdown();
void ForceRender();

// =============================================================================
// State
// =============================================================================
void  SetWantsScreenshot(bool v);
bool  WantsScreenshot();
void  SetWantsCRE(bool v);
bool  WantsCRE();

void  SetLastCapture(const std::wstring& filename, size_t size);
std::wstring GetLastCaptureFilename();
size_t GetLastCaptureSize();

// =============================================================================
// Settings
// =============================================================================
bool  GetIncludeInactive();
void  SetIncludeInactive(bool v);
bool  GetAutoOpenFolder();
void  SetAutoOpenFolder(bool v);

// =============================================================================
// Folder path
// =============================================================================
std::wstring GetSaveDirectoryPath();

} // namespace Gui
} // namespace NextCppShot
