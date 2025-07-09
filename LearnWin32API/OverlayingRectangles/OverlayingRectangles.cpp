#include <windows.h>
#include <gdiplus.h>
#include <vector>
#include <string>
#include <map>
#include <sstream>
#include <fstream>

#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "user32.lib")

// Structure to hold rectangle data
struct MyRect
{
    std::string name{};
    int top{}, left{}, right{}, bottom{};
    bool isPrimary{};
};

// Global variables
HINSTANCE g_hInstance;
HWND g_hwnd;
NOTIFYICONDATA g_nid = { sizeof(NOTIFYICONDATA) };
std::vector<MyRect> g_rectangles;
std::map<std::string, std::string> g_altNames;
Gdiplus::GdiplusStartupInput gdiplusStartupInput;
ULONG_PTR gdiplusToken;
std::string REG_PATH_RECTS = "";
std::string REG_PATH_ALT_NAMES = "";
const UINT WM_APP_TRAY = WM_APP + 1;

// Forward declarations
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
void LoadRegistryData();
void ShowErrorAndExit(const char* message);
void CreateTrayIcon(HWND hwnd);
void ShowContextMenu(HWND hwnd);
void DrawRectangles(HDC hdc);

std::string ReadOneLineFromFile(std::string const& fileName)
{
    std::ifstream inputFile(fileName); // Use ifstream for byte streams

    std::string line;
    if (inputFile.is_open())
    {
        if (std::getline(inputFile, line))
        {
            // Successfully read the first line
            return line;
        }
    }
    // Return an empty string if file cannot be opened or is empty
    return "";
}

// Initialize GDI+ and register window class
int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE, LPWSTR, int nCmdShow)
{
    Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, nullptr);
    g_hInstance = hInstance;

    WNDCLASSEX wc = { sizeof(WNDCLASSEX) };
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = "OverlayWindow";
    RegisterClassEx(&wc);

    // Get primary monitor dimensions
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);

    // Create window
    g_hwnd = CreateWindowEx(
        WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOPMOST,
        "OverlayWindow",
        "Overlay",
        WS_POPUP,
        0, 0, screenWidth, screenHeight,
        nullptr, nullptr, hInstance, nullptr
    );

    if (!g_hwnd)
    {
        ShowErrorAndExit("Failed to create window");
        return 1;
    }

    // Set window transparency (fully transparent background)
    SetLayeredWindowAttributes(g_hwnd, RGB(0, 0, 0), 0, LWA_COLORKEY);
    ShowWindow(g_hwnd, nCmdShow);
    UpdateWindow(g_hwnd);

    CreateTrayIcon(g_hwnd);
    LoadRegistryData();
    InvalidateRect(g_hwnd, nullptr, TRUE); // Force initial repaint
    SetTimer(g_hwnd, 1, 5000, nullptr); // 5-second registry polling

    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    Gdiplus::GdiplusShutdown(gdiplusToken);
    return (int)msg.wParam;
}

// Window procedure
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            // Fill background with transparent color
            HBRUSH hBrush = CreateSolidBrush(RGB(0, 0, 0));
            FillRect(hdc, &ps.rcPaint, hBrush);
            DeleteObject(hBrush);
            DrawRectangles(hdc);
            EndPaint(hwnd, &ps);
            return 0;
        }
        case WM_TIMER:
            LoadRegistryData();
            InvalidateRect(hwnd, nullptr, TRUE);
            UpdateWindow(hwnd); // Ensure immediate update
            return 0;
        case WM_APP_TRAY:
            if (lParam == WM_RBUTTONUP)
            {
                ShowContextMenu(hwnd);
            }
            return 0;
        case WM_COMMAND:
            if (LOWORD(wParam) == 1000)
            {
                Shell_NotifyIcon(NIM_DELETE, &g_nid);
                DestroyWindow(hwnd);
            }
            return 0;
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
            // Ignore mouse and keyboard input
        case WM_MOUSEMOVE:
        case WM_LBUTTONDOWN:
        case WM_RBUTTONDOWN:
        case WM_KEYDOWN:
        case WM_KEYUP:
            return 0;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

// Load rectangle and alternative name data from registry
void LoadRegistryData()
{
    g_rectangles.clear();
    g_altNames.clear();
    HKEY hKey;

    // Load primary rectangles
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, REG_PATH_RECTS.c_str(), 0, KEY_READ, &hKey) == ERROR_SUCCESS)
    {
        DWORD index = 0;
        char subKeyName[256];
        DWORD nameLen = sizeof(subKeyName) / sizeof(char);
        while (RegEnumKeyExA(hKey, index, subKeyName, &nameLen, nullptr, nullptr, nullptr, nullptr) == ERROR_SUCCESS)
        {
            HKEY hSubKey;
            std::string path = std::string(REG_PATH_RECTS) + "\\" + subKeyName;
            if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, path.c_str(), 0, KEY_READ, &hSubKey) == ERROR_SUCCESS)
            {
                MyRect rect;
                rect.name = subKeyName;
                rect.isPrimary = true;
                DWORD data{}, dataSize = sizeof(DWORD);
                bool valid = true;
                std::stringstream error;

                if (RegQueryValueEx(hSubKey, "Top", nullptr, nullptr, (LPBYTE)&data, &dataSize) != ERROR_SUCCESS || data > 100)
                {
                    error << "Invalid or missing 'Top' for " << subKeyName;
                    valid = false;
                }
                else
                {
                    rect.top = data;
                }
                if (RegQueryValueEx(hSubKey, "Left", nullptr, nullptr, (LPBYTE)&data, &dataSize) != ERROR_SUCCESS || data > 100)
                {
                    error << "Invalid or missing 'Left' for " << subKeyName;
                    valid = false;
                }
                else
                {
                    rect.left = data;
                }
                if (RegQueryValueEx(hSubKey, "Right", nullptr, nullptr, (LPBYTE)&data, &dataSize) != ERROR_SUCCESS || data > 100)
                {
                    error << "Invalid or missing 'Right' for " << subKeyName;
                    valid = false;
                }
                else
                {
                    rect.right = data;
                }
                if (RegQueryValueEx(hSubKey, "Bottom", nullptr, nullptr, (LPBYTE)&data, &dataSize) != ERROR_SUCCESS || data > 100)
                {
                    error << "Invalid or missing 'Bottom' for " << subKeyName;
                    valid = false;
                }
                else
                {
                    rect.bottom = data;
                }

                if (valid)
                {
                    g_rectangles.push_back(rect);
                }
                else
                {
                    RegCloseKey(hSubKey);
                    RegCloseKey(hKey);
                    ShowErrorAndExit(error.str().c_str());
                    return;
                }
                RegCloseKey(hSubKey);
            }
            nameLen = sizeof(subKeyName) / sizeof(wchar_t);
            index++;
        }
        RegCloseKey(hKey);
    }
    else
    {
        ShowErrorAndExit("Failed to open registry path for Rects");
        return;
    }

    // Load alternative names (optional)
    auto pathFromFile = ReadOneLineFromFile("fdklayout.txt");
    if (pathFromFile.size() > 0)
    {
        REG_PATH_ALT_NAMES = pathFromFile;
    }
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, REG_PATH_ALT_NAMES.c_str(), 0, KEY_READ, &hKey) == ERROR_SUCCESS)
    {
        DWORD index = 0;
        char valueName[256]{};
        DWORD nameLen = sizeof(valueName) / sizeof(char);
        char valueData[256]{};
        DWORD dataLen = sizeof(valueData);
        while (RegEnumValueA(hKey, index, valueName, &nameLen, nullptr, nullptr, (LPBYTE)valueData, &dataLen) == ERROR_SUCCESS)
        {
            char simplifyName[255]{};
            strcpy_s(simplifyName, 255, valueName + strlen(valueName) - 2);
            g_altNames[simplifyName] = valueData;
            nameLen = sizeof(valueName) / sizeof(wchar_t);
            dataLen = sizeof(valueData);
            index++;
        }
        RegCloseKey(hKey);
    }

    // Apply alternative names
    for (auto& rect : g_rectangles)
    {
        auto it = g_altNames.find(rect.name);
        if (it != g_altNames.end())
        {
            rect.name += " mapped to " + it->second;
            rect.isPrimary = false;
        }
    }
}

// Display error message and exit
void ShowErrorAndExit(const char* message)
{
    MessageBox(nullptr, message, "Error", MB_OK | MB_ICONERROR);
    if (g_hwnd)
    {
        Shell_NotifyIcon(NIM_DELETE, &g_nid);
        DestroyWindow(g_hwnd);
    }
    ExitProcess(1);
}

// Create system tray icon
void CreateTrayIcon(HWND hwnd)
{
    g_nid.hWnd = hwnd;
    g_nid.uID = 1;
    g_nid.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
    g_nid.uCallbackMessage = WM_APP_TRAY;
    g_nid.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    strcpy_s(g_nid.szTip, "Overlay Application");
    Shell_NotifyIcon(NIM_ADD, &g_nid);
}

// Show context menu for system tray
void ShowContextMenu(HWND hwnd)
{
    POINT pt;
    GetCursorPos(&pt);
    HMENU hMenu = CreatePopupMenu();
    AppendMenu(hMenu, MF_STRING, 1000, "Exit");
    SetForegroundWindow(hwnd);
    TrackPopupMenu(hMenu, TPM_RIGHTBUTTON, pt.x, pt.y, 0, hwnd, nullptr);
    DestroyMenu(hMenu);
}

std::wstring stringToWstringStream(const std::string& str)
{
    // This method is generally for simple conversions where the characters
    // in string and wstring are byte-compatible or handled by a basic locale.
    // It's not robust for complex encoding conversions like UTF-8 to UTF-16.
    std::wstringstream wss;
    wss << str.c_str(); // This might not correctly convert multi-byte characters
    return wss.str();
}

// Draw rectangles and labels
void DrawRectangles(HDC hdc)
{
    Gdiplus::Graphics graphics(hdc);
    graphics.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);

    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);

    for (const auto& rect : g_rectangles)
    {
        // Calculate rectangle coordinates
        int left = (rect.left * screenWidth) / 100;
        int top = (rect.top * screenHeight) / 100;
        int right = (rect.right * screenWidth) / 100;
        int bottom = (rect.bottom * screenHeight) / 100;

        // Draw rectangle border
        Gdiplus::Pen pen(rect.isPrimary ? Gdiplus::Color::Red : Gdiplus::Color::Blue, 2.0f);
        graphics.DrawRectangle(&pen, left, top, right - left, bottom - top);

        // Draw text
        Gdiplus::Font font(L"Consolas", 18, Gdiplus::FontStyleBold);
        Gdiplus::SolidBrush brush(rect.isPrimary ? Gdiplus::Color::Red : Gdiplus::Color::Blue);
        Gdiplus::StringFormat format;
        format.SetAlignment(Gdiplus::StringAlignmentCenter);
        format.SetLineAlignment(Gdiplus::StringAlignmentCenter);
        Gdiplus::RectF textRect((float)left, (float)top, (float)(right - left), (float)(bottom - top));
        graphics.DrawString(stringToWstringStream(rect.name.c_str()).c_str(), -1, &font, textRect, &format, &brush);
    }
}
