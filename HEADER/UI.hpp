#ifndef UI_HPP
#define UI_HPP

#ifndef UNICODE
#define UNICODE
#endif

#include <winsock2.h>
#include <windows.h>
#include <string>
#include <commctrl.h>
#include "ProxyServer.hpp"
#include <mutex>
#include "ThreadPool.hpp"
// ID của các controls
#define BTN_START 1
#define RADIO_MITM 2
#define RADIO_TRANSPARENT 3
#define BTN_LOG 4
#define BTN_HELP 5
#define BTN_BLACKLIST 6
#define EDIT_DISPLAY 7
#define EDIT_BLACKLIST 8
#define TITLE_BLACKLIST 9
#define BTN_SAVE 10
#define MAX_LINES 100
#define IDI_ICON 101

// // Trạng thái của button Start
// extern bool isStarted;

// // Các HWND toàn cục
// extern HWND hwndStart, hwndLog, hwndBlacklist, hwndHelp;
// extern HWND hwndGroupMode, hwndRadioMITM, hwndRadioTransparent;
// extern HWND hwndEditDisplay;
// extern HWND hListView;

// // Prototypes
// LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
// void DisplayContent(HWND hwnd, const std::wstring &content);
// void AppendContent(HWND hwndEdit, const std::wstring& content);
// void AppendList(HWND hwndList, const std::wstring col1, const std::wstring col2, const std::wstring col3);


struct PUI {
    std::mutex mtx; 
    static WNDCLASSEX wc;
    HWND hwndStart, hwndLog, hwndBlacklist, hwndHelp;
    HWND hwndGroupMode, hwndRadioMITM, hwndRadioTransparent;
    HWND hwndEdit;
    HWND hwndList;
    HWND hwndSave;
    HWND hwndPrevFocus = NULL;
    HBRUSH hbrBackground = CreateSolidBrush(RGB(220, 220, 220));

    HWND hwndTitleMessage, hwndTitleBlacklist;

    const int WINDOW_WIDTH = 900;
    const int WINDOW_HEIGHT = 515;
    const char *logFilePath = "./proxy_errors.log";
    const char *blacklistFilePath = "./CONFIG/blocked_sites.txt";

    bool isStarted = false;
    std::atomic<bool> isUpdatingLog = false;

    ProxyServer *proxy = nullptr;
    int type = -1;

    void disableEditing();
    void enableEditing();
    
    void disableUpdatingLog();
    void enableUpdatingLog();
    void StartLogUpdater();

    bool IsAtBottomEdit();
    bool IsAtBottomList();
    void ScrolltoEnd();

    PUI();
    void init(LRESULT CALLBACK (*WindowProc)(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam), HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow);
    void start();
    void DisplayEdit(const std::wstring &content);
    void AppendEdit(const std::wstring& content);
    void AppendList(const std::wstring col1, const std::wstring col2, const std::wstring col3);
};

int GetLineHeight(HWND hwnd);

extern PUI Window;
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

#endif