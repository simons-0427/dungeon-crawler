#include <windows.h>
#include "app_icon_resource.h"

void SetWindowsTaskbarIcon(void *windowHandle) {
    HWND window = (HWND)windowHandle;
    HINSTANCE instance = GetModuleHandleW(NULL);
    if (!window || !instance) return;

    HICON small = (HICON)LoadImageW(instance, MAKEINTRESOURCEW(IDI_APP_ICON), IMAGE_ICON,
                                    GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON),
                                    LR_DEFAULTCOLOR | LR_SHARED);
    HICON large = (HICON)LoadImageW(instance, MAKEINTRESOURCEW(IDI_APP_ICON), IMAGE_ICON,
                                    GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON),
                                    LR_DEFAULTCOLOR | LR_SHARED);
    if (small) {
        SetClassLongPtrW(window, GCLP_HICONSM, (LONG_PTR)small);
        SendMessageW(window, WM_SETICON, ICON_SMALL, (LPARAM)small);
    }
    if (large) {
        SetClassLongPtrW(window, GCLP_HICON, (LONG_PTR)large);
        SendMessageW(window, WM_SETICON, ICON_BIG, (LPARAM)large);
    }
}
