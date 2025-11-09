#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

extern "C" void *GetWindowHandle(void);

void SetWindowsTaskbarIconFromResource(){
    void* handle = GetWindowHandle();
    HWND hwnd = reinterpret_cast<HWND>(handle);
    if(!hwnd) return;
    HMODULE hShell = GetModuleHandleW(L"shell32.dll");
    if(!hShell) hShell = LoadLibraryW(L"shell32.dll");
    using SetAppIdFn = HRESULT (WINAPI*)(PCWSTR);
    if(hShell){
        auto setAppId = reinterpret_cast<SetAppIdFn>(GetProcAddress(hShell, "SetCurrentProcessExplicitAppUserModelID"));
        if(setAppId){ setAppId(L"com.shapey.tower"); }
    }
    HINSTANCE hInst = GetModuleHandle(nullptr);
    HICON hIconBig   = static_cast<HICON>(LoadImageW(hInst, MAKEINTRESOURCEW(101), IMAGE_ICON, 256, 256, LR_DEFAULTCOLOR));
    HICON hIconSmall = static_cast<HICON>(LoadImageW(hInst, MAKEINTRESOURCEW(101), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR));
    if(hIconBig)   SendMessage(hwnd, WM_SETICON, ICON_BIG,   reinterpret_cast<LPARAM>(hIconBig));
    if(hIconSmall) SendMessage(hwnd, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(hIconSmall));
}
#endif
