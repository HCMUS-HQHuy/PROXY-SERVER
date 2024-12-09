#ifndef UI_HPP
#define UI_HPP

#ifndef UNICODE
#define UNICODE
#endif

#include <winsock2.h>
#include <windows.h>
#include <string>

extern const int WINDOW_WIDTH;
extern const int WINDOW_HEIGHT;

// ID của các controls
#define BTN_START 1
#define RADIO_MITM 2
#define RADIO_TRANSPARENT 3
#define BTN_LOG 4
#define BTN_HELP 5
#define BTN_BLACKLIST 6
#define EDIT_DISPLAY 7
#define BTN_SAVE 8

// Trạng thái của button Start
extern bool isStarted;

// Các HWND toàn cục
extern HWND hwndStart, hwndLog, hwndBlacklist, hwndHelp;
extern HWND hwndGroupMode, hwndRadioMITM, hwndRadioTransparent;
extern HWND hwndEditDisplay;

// Prototypes
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void DisplayContent(HWND hwnd, const std::wstring &content);


#endif