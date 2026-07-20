#include <windows.h>
#include <commctrl.h>
#include <tchar.h>
#include <gdiplus.h>
#include <sstream>
#include <iostream>
#include <chrono>
#include <string>
#include <sys/stat.h>
#include <shellapi.h>

#include "resources.h"
#include "Utils.h"
#include "images/Screenshot.h"
#include "images/CompositeScreenshot.h"
#include "windows/BackdropWindow.h"

#include "gui/Gui.h"
#include "gui/renderer.h"

#define ERROR_TITLE L"NextCppShot Error"

// =============================================================================
// Backdrop Window Class Names
// =============================================================================
TCHAR blackBackdropClassName[] = _T("BlackBackdropWindow");
TCHAR whiteBackdropClassName[] = _T("WhiteBackdropWindow");

// =============================================================================
// Forward Declarations
// =============================================================================
void CaptureScreenshot(BackdropWindow& whiteWindow, BackdropWindow& blackWindow, bool creMode);

// =============================================================================
// Command Line Parsing
// =============================================================================
struct CommandLineOptions {
    bool useOldGui = false;
    bool showHelp = false;
    std::string renderBackend = "opengl3";  // default
};

void PrintHelp() {
    // Attach to parent console if running from terminal (WIN32 subsystem)
    if (AttachConsole(ATTACH_PARENT_PROCESS)) {
        // Disable stdout buffering so text appears immediately
        setvbuf(stdout, NULL, _IONBF, 0);
        FILE* fp;
        freopen_s(&fp, "CONOUT$", "w", stdout);
        freopen_s(&fp, "CONERR$", "w", stderr);
        // Ensure CRT handle is in sync
        _fileno(stdout) >= 0;
    }

    std::wcout << L"NextCppShot - Transparent Screenshot Utility\n\n";
    std::wcout << L"Usage: NextCppShot [OPTIONS]\n\n";
    std::wcout << L"Options:\n";
    std::wcout << L"  -o, --old-gui    Use the old Win32 GUI instead of ImGui\n";
    std::wcout << L"  -r, --render     Set render backend\n";
    std::wcout << L"  -h, --help       Show this help message\n\n";
    std::wcout << L"Render Backends:\n";
    std::wcout << L"  opengl3 (default)        OpenGL 3\n";
    std::wcout << L"  directx9                  DirectX 9\n";
    std::wcout << L"  directx10           DirectX 10 (not yet implemented)\n";
    std::wcout << L"  directx11           DirectX 11 (not yet implemented)\n";
    std::wcout << L"  directx12           DirectX 12 (not yet implemented)\n";
    std::wcout << L"  opengl2             OpenGL 2 (not yet implemented)\n";
    std::wcout << L"  vulkan              Vulkan (not yet implemented)\n\n";
    std::wcout << L"Keyboard Shortcuts:\n";
    std::wcout << L"  Ctrl+B           Take screenshot of active window\n";
    std::wcout << L"  Ctrl+Shift+B     Take CRE screenshot (active + inactive)\n";
    fflush(stdout);
}

CommandLineOptions ParseCommandLine(int argc, wchar_t* argv[]) {
    CommandLineOptions options;
    for (int i = 1; i < argc; i++) {
        std::wstring arg = argv[i];
        if (arg == L"-o" || arg == L"--old-gui")
            options.useOldGui = true;
        else if (arg == L"-h" || arg == L"--help")
            options.showHelp = true;
        else if (arg == L"-r" || arg == L"--render") {
            if (i + 1 < argc) {
                std::wstring backend = argv[i + 1];
                // Convert to lowercase
                for (auto& c : backend) c = towlower(c);
                if (backend == L"directx9" || backend == L"dx9" || backend == L"dx" || backend == L"directx")
                    options.renderBackend = "directx9";
                else if (backend == L"directx10" || backend == L"dx10")
                    options.renderBackend = "directx10";
                else if (backend == L"directx11" || backend == L"dx11")
                    options.renderBackend = "directx11";
                else if (backend == L"directx12" || backend == L"dx12")
                    options.renderBackend = "directx12";
                else if (backend == L"opengl2" || backend == L"gl2")
                    options.renderBackend = "opengl2";
                else if (backend == L"opengl" || backend == L"opengl3" || backend == L"gl" || backend == L"gl3")
                    options.renderBackend = "opengl3";
                else if (backend == L"vulkan" || backend == L"vk")
                    options.renderBackend = "vulkan";
                else
                    std::wcerr << L"Unknown render backend: " << backend << L", using default (directx9)" << std::endl;
                i++; // skip next arg
            }
        }
    }
    return options;
}

// =============================================================================
// Utility Functions
// =============================================================================
inline bool FileExists(const std::wstring& name) {
    return GetFileAttributes(name.c_str()) != INVALID_FILE_ATTRIBUTES && GetLastError() != ERROR_FILE_NOT_FOUND;
}

void RemoveIllegalChars(std::wstring& str) {
    std::wstring illegalChars = L"\\/:?\"<>|*";
    for (auto it = str.begin(); it < str.end(); ++it) {
        bool found = illegalChars.find(*it) != std::wstring::npos;
        if (found)
            *it = ' ';
    }
}

std::wstring GetSafeFilenameBase(std::wstring windowTitle) {
    RemoveIllegalChars(windowTitle);

    std::wstring path = CppShot::getSaveDirectory();
    std::wcout << L"registrypath: " << path << std::endl;

    CreateDirectory(path.c_str(), NULL);

    std::wstringstream pathbuild;

    std::wstring fileNameBase;

    unsigned int i = 0;
    do {
        pathbuild.str(L"");
        pathbuild << path << L"\\" << windowTitle << L"_" << i;
        fileNameBase = pathbuild.str();
        i++;
    } while(FileExists(fileNameBase + L"_b1.png") || FileExists(fileNameBase + L"_b2.png"));

    return fileNameBase;
}

// =============================================================================
// Screenshot Capture
// =============================================================================
void CaptureScreenshot(BackdropWindow& whiteWindow, BackdropWindow& blackWindow, bool creMode) {
    std::cout << "Screenshot capture start: " << CppShot::currentTimestamp() << std::endl;

    HWND desktopWindow = GetDesktopWindow();
    HWND foregroundWindow = GetForegroundWindow();
    HWND taskbar = FindWindow(L"Shell_TrayWnd", NULL);
    HWND startButton = FindWindow(L"Button", L"Start");

    std::pair<Screenshot, Screenshot> shots;
    std::pair<Screenshot, Screenshot> creShots;

    if (foregroundWindow != taskbar && foregroundWindow != startButton) {
        ShowWindow(taskbar, 0);
        ShowWindow(startButton, 0);
    }

    whiteWindow.resize(foregroundWindow);
    blackWindow.resize(foregroundWindow);

    SetForegroundWindow(foregroundWindow);

    std::cout << "Additional white flash: " << CppShot::currentTimestamp() << std::endl;

    blackWindow.hide();
    whiteWindow.show();

    std::cout << "Capturing black: " << CppShot::currentTimestamp() << std::endl;
    whiteWindow.hide();
    blackWindow.show();
    shots.second.capture(foregroundWindow);

    std::cout << "Capturing white: " << CppShot::currentTimestamp() << std::endl;
    blackWindow.hide();
    whiteWindow.show();
    shots.first.capture(foregroundWindow);

    if (creMode) {
        SetForegroundWindow(desktopWindow);
        Sleep(33);
        creShots.first.capture(foregroundWindow);

        std::cout << "Capturing black inactive: " << CppShot::currentTimestamp() << std::endl;
        whiteWindow.hide();
        blackWindow.show();
        creShots.second.capture(foregroundWindow);
    }

    ShowWindow(taskbar, 1);
    ShowWindow(startButton, 1);

    blackWindow.hide();
    whiteWindow.hide();

    if (!shots.first.isCaptured() || !shots.second.isCaptured()) {
        MessageBox(NULL, L"Screenshot is empty, aborting capture.", ERROR_TITLE, MB_OK | MB_ICONSTOP);
        return;
    }

    std::cout << "Starting image save: " << CppShot::currentTimestamp() << std::endl;
    std::cout << "Saving: " << CppShot::currentTimestamp() << std::endl;

    TCHAR h[2048];
    GetWindowText(foregroundWindow, h, 2048);
    std::wstring windowTextStr(h);

    auto base = GetSafeFilenameBase(windowTextStr);

    std::cout << "Differentiating alpha: " << CppShot::currentTimestamp() << std::endl;
    try {
        CompositeScreenshot transparentImage(shots.first, shots.second);
        transparentImage.save(base + L"_b1.png");

        if (creShots.first.isCaptured() && creShots.second.isCaptured()) {
            CompositeScreenshot transparentInactiveImage(creShots.first, creShots.second, transparentImage.getCrop());
            std::cout << "Inactive image ptr: " << transparentInactiveImage.getBitmap() << std::endl;
            std::cout << transparentInactiveImage.getBitmap()->GetWidth() << "x" << transparentInactiveImage.getBitmap()->GetHeight() << std::endl;
            transparentInactiveImage.save(base + L"_b2.png");
        }
    }
    catch (std::runtime_error& e) {
        MessageBox(NULL, L"An error has occured while capturing the screenshot.", ERROR_TITLE, MB_OK | MB_ICONSTOP);
        return;
    }

    std::wstring filename = base + (creShots.first.isCaptured() ? L"_b2.png" : L"_b1.png");
    WIN32_FILE_ATTRIBUTE_DATA fileInfo;
    GetFileAttributesEx(filename.c_str(), GetFileExInfoStandard, &fileInfo);
    size_t fileSize = fileInfo.nFileSizeHigh * (MAXWORD + 1) + fileInfo.nFileSizeLow;
    NextCppShot::Gui::SetLastCapture(filename, fileSize);
}

// =============================================================================
// Exception Handler
// =============================================================================
static LONG WINAPI exceptionHandler(LPEXCEPTION_POINTERS info) {
    HWND taskbar = FindWindow(L"Shell_TrayWnd", NULL);
    HWND startButton = FindWindow(L"Button", L"Start");

    ShowWindow(taskbar, 1);
    ShowWindow(startButton, 1);

    std::wstringstream ss;
    ss << L"An unhandled exception has occured. The program will now terminate.\n\n";
    ss << L"Exception code: 0x" << std::hex << info->ExceptionRecord->ExceptionCode << std::dec << L"\n";
    ss << L"Exception address: 0x" << std::hex << info->ExceptionRecord->ExceptionAddress << std::dec << L"\n";
    MessageBox(NULL, ss.str().c_str(), ERROR_TITLE, MB_OK | MB_ICONSTOP);
    return EXCEPTION_CONTINUE_SEARCH;
}

// =============================================================================
// OLD GUI (Win32 Native)
// =============================================================================
#include "windows/MainWindow.h"
#include "ui/Button.h"

int RunOldGui(HINSTANCE hThisInstance, int nCmdShow) {
    std::cout << "Starting with OLD GUI (Win32)\n";

    MainWindow window;
    window.show(nCmdShow);

    if (!RegisterHotKey(NULL, 1, MOD_CONTROL, 'B')) {
        MessageBox(NULL, L"Unable to register the CTRL+B keyboard shortcut.", ERROR_TITLE, 0x10);
    }
    if (!RegisterHotKey(NULL, 2, MOD_CONTROL | MOD_SHIFT, 'B')) {
        MessageBox(NULL, L"Unable to register the CTRL+SHIFT+B keyboard shortcut.", ERROR_TITLE, 0x10);
    }

    BackdropWindow whiteWindow(RGB(255, 255, 255), whiteBackdropClassName);
    BackdropWindow blackWindow(RGB(0, 0, 0), blackBackdropClassName);

    Gdiplus::GdiplusStartupInput gpStartupInput;
    ULONG_PTR gpToken;
    Gdiplus::GdiplusStartup(&gpToken, &gpStartupInput, NULL);

    MSG messages;
    while (GetMessage(&messages, NULL, 0, 0)) {
        if (messages.message == WM_HOTKEY) {
            _tprintf(_T("WM_HOTKEY received\n"));
            if (messages.wParam == 1)
                CaptureScreenshot(whiteWindow, blackWindow, false);
            else if (messages.wParam == 2)
                CaptureScreenshot(whiteWindow, blackWindow, true);
        }
        TranslateMessage(&messages);
        DispatchMessage(&messages);
    }

    Gdiplus::GdiplusShutdown(gpToken);
    UnregisterHotKey(NULL, 1);
    UnregisterHotKey(NULL, 2);

    return messages.wParam;
}

// =============================================================================
// NEW GUI (Dear ImGui)
// =============================================================================
int RunNewGui(HINSTANCE hThisInstance, int nCmdShow, const CommandLineOptions& options) {
    std::cout << "Starting with NEW GUI (Dear ImGui)\n";
    std::cout << "Render backend: " << options.renderBackend << std::endl;

    // Convert CLI string to enum
    std::wstring backendW(options.renderBackend.begin(), options.renderBackend.end());
    NextCppShot::RendererType rtype = NextCppShot::RendererTypeFromString(backendW.c_str());

    // Create renderer via factory
    std::unique_ptr<NextCppShot::Renderer> renderer(NextCppShot::CreateRenderer(rtype));
    if (!renderer) {
        MessageBox(NULL, L"Failed to create renderer (factory returned null).",
                   ERROR_TITLE, MB_ICONERROR);
        return 1;
    }
    NextCppShot::Renderer::SetCurrent(renderer.get());

    NextCppShot::Gui::InitWindow(hThisInstance, nCmdShow);

    Gdiplus::GdiplusStartupInput gpStartupInput;
    ULONG_PTR gpToken;
    Gdiplus::GdiplusStartup(&gpToken, &gpStartupInput, NULL);

    if (!NextCppShot::Gui::Init()) {
        MessageBox(NULL, L"Failed to initialize GUI", ERROR_TITLE, MB_ICONERROR);
        Gdiplus::GdiplusShutdown(gpToken);
        NextCppShot::Renderer::SetCurrent(nullptr);
        return 1;
    }

    if (!RegisterHotKey(NULL, 1, MOD_CONTROL, 'B')) {
        MessageBox(NULL, L"Unable to register CTRL+B shortcut.", ERROR_TITLE, MB_ICONWARNING);
    }
    if (!RegisterHotKey(NULL, 2, MOD_CONTROL | MOD_SHIFT, 'B')) {
        MessageBox(NULL, L"Unable to register CTRL+SHIFT+B shortcut.", ERROR_TITLE, MB_ICONWARNING);
    }

    BackdropWindow whiteWindow(RGB(255, 255, 255), whiteBackdropClassName);
    BackdropWindow blackWindow(RGB(0, 0, 0), blackBackdropClassName);

    MSG msg;
    bool done = false;
    while (!done) {
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) {
                done = true;
                break;
            }
            if (msg.message == WM_HOTKEY) {
                _tprintf(_T("WM_HOTKEY received: %d\n"), msg.wParam);
                if (msg.wParam == 1)
                    NextCppShot::Gui::SetWantsScreenshot(true);
                else if (msg.wParam == 2)
                    NextCppShot::Gui::SetWantsCRE(true);
            }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        if (done) break;

        if (NextCppShot::Gui::WantsScreenshot()) {
            NextCppShot::Gui::SetWantsScreenshot(false);
            CaptureScreenshot(whiteWindow, blackWindow, false);
        }
        if (NextCppShot::Gui::WantsCRE()) {
            NextCppShot::Gui::SetWantsCRE(false);
            CaptureScreenshot(whiteWindow, blackWindow, true);
        }

        if (!IsIconic(NextCppShot::Gui::g_hwnd)) {
            NextCppShot::Gui::ForceRender();
        }
    }

    NextCppShot::Gui::Shutdown();
    Gdiplus::GdiplusShutdown(gpToken);
    UnregisterHotKey(NULL, 1);
    UnregisterHotKey(NULL, 2);

    // Release renderer
    NextCppShot::Renderer::SetCurrent(nullptr);
    renderer.reset();

    return 0;
}

// =============================================================================
// Main Entry Point
// =============================================================================
int WINAPI WinMain(HINSTANCE hThisInstance,
                   HINSTANCE hPrevInstance,
                   LPSTR lpszArgument,
                   int nCmdShow)
{
    SetUnhandledExceptionFilter(exceptionHandler);

    int argc = 0;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (!argv) return 1;

    CommandLineOptions options = ParseCommandLine(argc, argv);
    LocalFree(argv);

    if (options.showHelp) {
        PrintHelp();
        return 0;
    }

    if (options.useOldGui) {
        return RunOldGui(hThisInstance, nCmdShow);
    }
    else {
        return RunNewGui(hThisInstance, nCmdShow, options);
    }
}
