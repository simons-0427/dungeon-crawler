#include "game_console_win32.h"
#include <stdio.h>

#ifdef _WIN32
#include <windows.h>
#include <shellapi.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

// These two raylib logging entry points are declared here to keep raylib.h and
// windows.h out of the same translation unit (their UI names conflict).
extern void TraceLog(int logLevel, const char *text, ...);
extern void SetTraceLogCallback(void (*callback)(int, const char *, va_list));
extern void SetTraceLogLevel(int logLevel);
enum { GAME_LOG_DEBUG = 2, GAME_LOG_INFO = 3, GAME_LOG_WARNING = 4, GAME_LOG_ERROR = 5 };

static HANDLE logFile = INVALID_HANDLE_VALUE;
static HANDLE viewerProcess = NULL;
static char logPath[MAX_PATH];

static bool BuildLogPath(void) {
    char exePath[MAX_PATH];
    DWORD len = GetModuleFileNameA(NULL, exePath, MAX_PATH);
    if (!len || len >= MAX_PATH) return false;
    char *slash = strrchr(exePath, '\\');
    if (!slash) return false;
    slash[1] = '\0';
#ifdef DEBUG_ROOMS
    const char *name = "saves\\dungeon_console.log";
#else
    const char *name = "saves\\dungeon_console.log";
#endif
    if (strlen(exePath) + strlen(name) >= sizeof(logPath)) return false;
    strcpy(logPath, exePath);
    strcat(logPath, name);
    return true;
}

static void WriteLog(const char *message, size_t length) {
    if (logFile == INVALID_HANDLE_VALUE || !message || !length) return;
    DWORD written;
    WriteFile(logFile, message, (DWORD)length, &written, NULL);
}

static void GameTraceLog(int level, const char *format, va_list args) {
    char message[2048];
    const char *label = level == GAME_LOG_ERROR ? "ERROR" : level == GAME_LOG_WARNING ? "WARN"
                        : level == GAME_LOG_INFO ? "INFO" : "DEBUG";
    int offset = snprintf(message, sizeof(message), "[%s] ", label);
    if (offset < 0 || offset >= (int)sizeof(message)) return;
    int length = vsnprintf(message + offset, sizeof(message) - (size_t)offset, format, args);
    if (length < 0) return;
    size_t total = (size_t)offset + (size_t)length;
    if (total > sizeof(message) - 2) total = sizeof(message) - 2;
    message[total++] = '\r';
    message[total++] = '\n';
    WriteLog(message, total);
}

void GameConsoleInit(void) {
    if (!BuildLogPath()) return;
    char dir[MAX_PATH];
    strcpy(dir, logPath);
    char *slash = strrchr(dir, '\\');
    if (slash) {
        *slash = '\0';
        CreateDirectoryA(dir, NULL);
    }
    logFile = CreateFileA(logPath, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
                          NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (logFile == INVALID_HANDLE_VALUE) return;
    SetTraceLogCallback(GameTraceLog);
    TraceLog(GAME_LOG_INFO, "Dungeon Crawler started (PID %lu)", (unsigned long)GetCurrentProcessId());
}

static void StopViewer(void) {
    if (!viewerProcess) return;
    if (WaitForSingleObject(viewerProcess, 0) == WAIT_TIMEOUT) {
        TerminateProcess(viewerProcess, 0);
        WaitForSingleObject(viewerProcess, 1000);
    }
    CloseHandle(viewerProcess);
    viewerProcess = NULL;
}

bool GameConsoleSetEnabled(bool enabled) {
    if (!enabled) { StopViewer(); return true; }
    if (viewerProcess && WaitForSingleObject(viewerProcess, 0) == WAIT_TIMEOUT) return true;
    StopViewer();
    if (logFile == INVALID_HANDLE_VALUE) return false;
    char exePath[MAX_PATH], command[MAX_PATH + 64];
    DWORD len = GetModuleFileNameA(NULL, exePath, MAX_PATH);
    if (!len || len >= MAX_PATH) return false;
    snprintf(command, sizeof(command), "\"%s\" --console-viewer %lu", exePath,
             (unsigned long)GetCurrentProcessId());
    STARTUPINFOA startup = {0};
    PROCESS_INFORMATION process = {0};
    startup.cb = sizeof(startup);
    if (!CreateProcessA(exePath, command, NULL, NULL, FALSE, 0, NULL, NULL,
                        &startup, &process)) {
        TraceLog(GAME_LOG_WARNING, "Could not open Windows console: %lu", GetLastError());
        return false;
    }
    CloseHandle(process.hThread);
    viewerProcess = process.hProcess;
    return true;
}

bool GameConsoleUpdate(void) {
    if (!viewerProcess || WaitForSingleObject(viewerProcess, 0) == WAIT_TIMEOUT) return false;
    CloseHandle(viewerProcess);
    viewerProcess = NULL;
    return true;
}

void GameConsoleShutdown(void) {
    StopViewer();
    SetTraceLogCallback(NULL);
    if (logFile != INVALID_HANDLE_VALUE) CloseHandle(logFile);
    logFile = INVALID_HANDLE_VALUE;
}

void GameConsoleSetLogDetail(int detail) {
    static const int minimumLevel[] = {
        GAME_LOG_ERROR, GAME_LOG_WARNING, GAME_LOG_INFO, GAME_LOG_DEBUG
    };
    if (detail < 0 || detail > 3) detail = 2;
    SetTraceLogLevel(minimumLevel[detail]);
}

bool GameConsoleOpenLogFile(void) {
    if (!logPath[0] || logFile == INVALID_HANDLE_VALUE) return false;
    char parameters[MAX_PATH + 3];
    snprintf(parameters, sizeof(parameters), "\"%s\"", logPath);
    return (INT_PTR)ShellExecuteA(NULL, "open", "notepad.exe", parameters,
                                  NULL, SW_SHOWNORMAL) > 32;
}

bool GameConsoleOpenUrl(const char *url) {
    if (!url || strncmp(url, "https://", 8) != 0) return false;
    return (INT_PTR)ShellExecuteA(NULL, "open", url, NULL, NULL, SW_SHOWNORMAL) > 32;
}

void GameConsoleGetSystemInfo(char *out, size_t capacity) {
    if (!out || capacity == 0) return;
    SYSTEM_INFO system;
    MEMORYSTATUSEX memory;
    DISPLAY_DEVICEA display;
    GetSystemInfo(&system);
    memset(&memory, 0, sizeof(memory));
    memory.dwLength = sizeof(memory);
    bool hasMemory = GlobalMemoryStatusEx(&memory) != 0;
    char ram[48] = "Unknown";
    if (hasMemory)
        snprintf(ram, sizeof(ram), "%.1f GiB",
                 (double)memory.ullTotalPhys / (1024.0 * 1024.0 * 1024.0));
    const char *graphics = "Unknown";
    for (DWORD index = 0; index < 16; index++) {
        memset(&display, 0, sizeof(display));
        display.cb = sizeof(display);
        if (!EnumDisplayDevicesA(NULL, index, &display, 0)) break;
        if (display.StateFlags & DISPLAY_DEVICE_PRIMARY_DEVICE) {
            graphics = display.DeviceString;
            break;
        }
    }
    snprintf(out, capacity,
             "System: Windows\nLogical CPU processors: %lu\nPhysical RAM: %s\nGraphics adapter: %s\n",
             (unsigned long)system.dwNumberOfProcessors, ram, graphics);
}

bool GameConsoleRunViewerIfRequested(int argc, char **argv) {
    if (argc < 2 || strcmp(argv[1], "--console-viewer") != 0) return false;
    if (argc < 3) return true;
    DWORD parentPid = (DWORD)strtoul(argv[2], NULL, 10);
    HANDLE parent = OpenProcess(SYNCHRONIZE, FALSE, parentPid);
    if (!parent || !BuildLogPath() || !AllocConsole()) {
        if (parent) CloseHandle(parent);
        return true;
    }
    SetConsoleTitleA("Dungeon Crawler - Windows Console");
    HANDLE output = GetStdHandle(STD_OUTPUT_HANDLE);
    const char *heading = "Dungeon Crawler diagnostics\r\nClose this window to hide the console.\r\n\r\n";
    DWORD written;
    WriteFile(output, heading, (DWORD)strlen(heading), &written, NULL);

    HANDLE input = INVALID_HANDLE_VALUE;
    while (WaitForSingleObject(parent, 0) == WAIT_TIMEOUT) {
        if (input == INVALID_HANDLE_VALUE)
            input = CreateFileA(logPath, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
                                NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (input != INVALID_HANDLE_VALUE) {
            char buffer[2048];
            DWORD count = 0;
            if (ReadFile(input, buffer, sizeof(buffer), &count, NULL) && count)
                WriteFile(output, buffer, count, &written, NULL);
        }
        Sleep(100);
    }
    if (input != INVALID_HANDLE_VALUE) CloseHandle(input);
    CloseHandle(parent);
    return true;
}

#else
bool GameConsoleRunViewerIfRequested(int argc, char **argv) { (void)argc; (void)argv; return false; }
void GameConsoleInit(void) {}
bool GameConsoleSetEnabled(bool enabled) { return !enabled; }
bool GameConsoleUpdate(void) { return false; }
void GameConsoleShutdown(void) {}
void GameConsoleSetLogDetail(int detail) { (void)detail; }
bool GameConsoleOpenLogFile(void) { return false; }
bool GameConsoleOpenUrl(const char *url) { (void)url; return false; }
void GameConsoleGetSystemInfo(char *out, size_t capacity) {
    if (out && capacity) snprintf(out, capacity, "System: non-Windows\n");
}
#endif
