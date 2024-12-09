#include "../../HEADER/UI.hpp"
#include <fstream>
#include <thread>
#include <chrono>
#include <atomic>
#include <bits/shared_ptr.h>
#include "../../HEADER/ProxyServer.hpp"
#include "../../HEADER/Setting.hpp"
#include <string.h>
#include <commctrl.h>

const int WINDOW_WIDTH = 900;
const int WINDOW_HEIGHT = 500;
const char *logFilePath = "./proxy_errors.log";
const char *blacklistFilePath = "./CONFIG/blocked_sites.txt";

bool isStarted = false;
std::atomic<bool> isUpdatingLog(false);

HWND hwndStart, hwndLog, hwndBlacklist, hwndHelp;
HWND hwndGroupMode, hwndRadioMITM, hwndRadioTransparent;
HWND hwndEditDisplay, hwndSave;
HWND hListView;

std::shared_ptr<ProxyServer> proxy;
int type = -1;
HBRUSH hbrBackground = CreateSolidBrush(RGB(192, 192, 192));

int GetLineHeight(HWND hwndEdit) {
    HDC hdc = GetDC(hwndEdit);
    HFONT hFont = (HFONT)SendMessage(hwndEdit, WM_GETFONT, 0, 0);
    HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);

    TEXTMETRIC tm;
    GetTextMetrics(hdc, &tm);

    SelectObject(hdc, hOldFont);
    ReleaseDC(hwndEdit, hdc);

    return tm.tmHeight;
}

bool IsAtBottom(HWND hwndEdit) {
    int totalLines = SendMessage(hwndEdit, EM_GETLINECOUNT, 0, 0);
    int firstVisible = SendMessage(hwndEdit, EM_GETFIRSTVISIBLELINE, 0, 0);

    RECT rect;
    GetClientRect(hwndEdit, &rect);

    int visibleLines = rect.bottom / GetLineHeight(hwndEdit);
    return (firstVisible + visibleLines >= totalLines - 1);
}


std::wstring ConvertToCRLF(const std::wstring& input) {
    std::wstring output;
    output.reserve(input.size());
    for (wchar_t ch : input) {
        if (ch == L'\n') {
            output += L"\r\n";
        } else {
            output += ch;
        }
    }
    return output;
}

void SaveFile(const std::string& filePath, const std::wstring& content) {
    std::wofstream file(filePath, std::ios::trunc);
    if (file.is_open()) {
        file << content;
        file.close();
    }
}

void disableEditing() {
    SendMessage(hwndEditDisplay, EM_SETREADONLY, TRUE, 0);
    EnableWindow(hwndSave, FALSE);
}

void enableEditing() {
    SendMessage(hwndEditDisplay, EM_SETREADONLY, FALSE, 0);
    EnableWindow(hwndSave, TRUE);
}

void disableUpdatingLog() {
    isUpdatingLog = false;
}

void enableUpdatingLog() {
    isUpdatingLog = true;
}

// void StartLogUpdater(HWND hwnd) {
//     std::thread([hwnd]() {
//         while (isViewingLog) {
//             std::wifstream file("log.txt");
//             std::wstring conten((std::istreambuf_iterator<wchar_t>(file)), std::istreambuf_iterator<wchar_t>());
//             DisplayContent(hwnd, content);
//             std::this_thread::sleep_for(std::chrono::seconds(1));
//         }
//     }).detach();
// }
void AppendContent(HWND hwndEdit, const std::wstring& content) {
    // Tạm thời tắt việc vẽ lại để tránh giật
    SendMessage(hwndEdit, WM_SETREDRAW, FALSE, 0);

    // Lưu trạng thái cuộn và vị trí chọn
    int curPos = SendMessage(hwndEdit, EM_GETFIRSTVISIBLELINE, 0, 0);
    bool atBottom = IsAtBottom(hwndEdit);
    int left, right;
    SendMessage(hwndEdit, EM_GETSEL, (WPARAM)&left, (LPARAM)&right);

    // Di chuyển con trỏ đến cuối và chèn nội dung mới
    int nLength = GetWindowTextLength(hwndEdit);
    SendMessage(hwndEdit, EM_SETSEL, (WPARAM)nLength, (LPARAM)nLength);
    SendMessage(hwndEdit, EM_REPLACESEL, (WPARAM)TRUE, (LPARAM)ConvertToCRLF(content).c_str());
    SendMessage(hwndEdit, EM_SETSEL, left, right);

    // Điều chỉnh lại vị trí cuộn
    if (atBottom) {
        SendMessage(hwndEdit, EM_LINESCROLL, 0, SendMessage(hwndEdit, EM_GETLINECOUNT, 0, 0));
    } else {
        SendMessage(hwndEdit, EM_LINESCROLL, 0, curPos - SendMessage(hwndEdit, EM_GETFIRSTVISIBLELINE, 0, 0));
    }

    // Bật lại việc vẽ lại và làm mới cửa sổ
    SendMessage(hwndEdit, WM_SETREDRAW, TRUE, 0);
    InvalidateRect(hwndEdit, NULL, TRUE);
}

void DisplayContent(HWND hwndEdit, const std::wstring& content) {
    SetWindowText(hwndEdit, ConvertToCRLF(content).c_str());
}


void StartLogUpdater(HWND hwndEdit) {
    isUpdatingLog = true; // Bắt đầu cập nhật log

    std::thread([hwndEdit]() {
        while (isUpdatingLog) {
            // Đọc nội dung từ file log
            std::wifstream file(logFilePath);
            if (file.is_open()) {
                std::wstring content((std::istreambuf_iterator<wchar_t>(file)), std::istreambuf_iterator<wchar_t>());
                file.close();

                // Hiển thị nội dung log
                DisplayContent(hwndEdit, L"Hello\r\n");
            }
            // Đợi 1 giây trước lần cập nhật tiếp theo
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }).detach(); // Tách luồng để chạy độc lập
}



LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_CREATE: {
//         hwndStart = CreateWindow(L"BUTTON", L"Start", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
//             20, 20, 80, 30, hwnd, (HMENU)BTN_START, (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), NULL);
//         hwndLog = CreateWindow(L"BUTTON", L"Log", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
//             120, 20, 80, 30, hwnd, (HMENU)BTN_LOG, (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), NULL);
//         hwndBlacklist = CreateWindow(L"BUTTON", L"Blacklist", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
//             20, 60, 80, 30, hwnd, (HMENU)BTN_BLACKLIST, (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), NULL);
//         hwndHelp = CreateWindow(L"BUTTON", L"Help", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
//             120, 60, 80, 30, hwnd, (HMENU)BTN_HELP, (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), NULL);
//         hwndGroupMode = CreateWindow(L"BUTTON", L"Choose Mode", WS_VISIBLE | WS_CHILD | BS_GROUPBOX,
//             20, 110, 180, 100, hwnd, NULL, (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), NULL);
//         hwndRadioMITM = CreateWindow(L"BUTTON", L"MITM", WS_VISIBLE | WS_CHILD | BS_RADIOBUTTON,
//             30, 140, 120, 20, hwnd, (HMENU)RADIO_MITM, (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), NULL);
//         hwndRadioTransparent = CreateWindow(L"BUTTON", L"Transparent", WS_VISIBLE | WS_CHILD | BS_RADIOBUTTON,
//             30, 170, 120, 20, hwnd, (HMENU)RADIO_TRANSPARENT, (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), NULL);
//         hwndEditDisplay = CreateWindow(L"EDIT", L"Content", WS_VISIBLE | WS_CHILD | WS_VSCROLL | ES_MULTILINE | ES_AUTOVSCROLL,
//             220, 20, 460, 300, hwnd, (HMENU)EDIT_DISPLAY, (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), NULL);
//         hwndSave = CreateWindow(L"BUTTON", L"Save", WS_VISIBLE | WS_CHILD | WS_DISABLED | BS_PUSHBUTTON,
//             20, 220, 80, 30, hwnd, (HMENU)BTN_SAVE, (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), NULL);


// // Tạo ListView
//         hListView = CreateWindowEx(
//             0, WC_LISTVIEW, NULL,
//             WS_CHILD | WS_VISIBLE | LVS_REPORT, // Chế độ "Report View"
//             10, 10, 600, 400,
//             hwnd, (HMENU)1, GetModuleHandle(NULL), NULL);

//         // Thêm các cột vào ListView
//         LVCOLUMN lvColumn;
//         lvColumn.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;

//         lvColumn.pszText = (LPWSTR)L"Column 1";
//         lvColumn.cx = 100;
//         lvColumn.iSubItem = 0;
//         ListView_InsertColumn(hListView, 0, &lvColumn);

//         lvColumn.pszText = (LPWSTR)L"Column 2";
//         lvColumn.cx = 150;
//         lvColumn.iSubItem = 1;
//         ListView_InsertColumn(hListView, 1, &lvColumn);

//         lvColumn.pszText = (LPWSTR)L"Column 3";
//         lvColumn.cx = 200;
//         lvColumn.iSubItem = 2;
//         ListView_InsertColumn(hListView, 2, &lvColumn);

//         // Thêm các dòng vào ListView
//         LVITEM lvItem;
//         lvItem.mask = LVIF_TEXT;

//         lvItem.iItem = 0; // Dòng đầu tiên
//         lvItem.iSubItem = 0; // Cột đầu tiên
//         lvItem.pszText = (LPWSTR)L"Row 1, Col 1";
//         ListView_InsertItem(hListView, &lvItem);

//         // Thêm dữ liệu vào các cột khác của dòng đầu tiên
//         ListView_SetItemText(hListView, 0, 1, (LPWSTR)L"Row 1, Col 2");
//         ListView_SetItemText(hListView, 0, 2, (LPWSTR)L"Row 1, Col 3");

//         // Thêm dòng thứ hai
//         lvItem.iItem = 1; // Dòng thứ hai
//         lvItem.iSubItem = 0; // Cột đầu tiên
//         lvItem.pszText = (LPWSTR)L"Row 2, Col 1";
//         ListView_InsertItem(hListView, &lvItem);

//         // Thêm dữ liệu vào các cột khác của dòng thứ hai
//         ListView_SetItemText(hListView, 1, 1, (LPWSTR)L"Row 2, Col 2");
//         ListView_SetItemText(hListView, 1, 2, (LPWSTR)L"Row 2, Col 3");

//         break;
//     }
        // Tạo các thành phần con
        hwndStart = CreateWindow(L"BUTTON", L"Start", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
            20, 20, 80, 30, hwnd, (HMENU)BTN_START, (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), NULL);
        hwndLog = CreateWindow(L"BUTTON", L"Log", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            120, 20, 80, 30, hwnd, (HMENU)BTN_LOG, (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), NULL);
        hwndBlacklist = CreateWindow(L"BUTTON", L"Blacklist", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            20, 60, 80, 30, hwnd, (HMENU)BTN_BLACKLIST, (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), NULL);
        hwndHelp = CreateWindow(L"BUTTON", L"Help", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            120, 60, 80, 30, hwnd, (HMENU)BTN_HELP, (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), NULL);
        hwndGroupMode = CreateWindow(L"BUTTON", L"Choose Mode", WS_VISIBLE | WS_CHILD | BS_GROUPBOX,
            20, 110, 180, 100, hwnd, NULL, (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), NULL);
        hwndRadioMITM = CreateWindow(L"BUTTON", L"MITM", WS_VISIBLE | WS_CHILD | BS_RADIOBUTTON,
            30, 140, 120, 20, hwnd, (HMENU)RADIO_MITM, (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), NULL);
        hwndRadioTransparent = CreateWindow(L"BUTTON", L"Transparent", WS_VISIBLE | WS_CHILD | BS_RADIOBUTTON,
            30, 170, 120, 20, hwnd, (HMENU)RADIO_TRANSPARENT, (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), NULL);
        hwndEditDisplay = CreateWindow(L"EDIT", L"Content", WS_VISIBLE | WS_CHILD | WS_VSCROLL | ES_MULTILINE | ES_AUTOVSCROLL,
            220, 20, 640, 200, hwnd, (HMENU)EDIT_DISPLAY, (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), NULL);
        hwndSave = CreateWindow(L"BUTTON", L"Save", WS_VISIBLE | WS_CHILD | WS_DISABLED | BS_PUSHBUTTON,
            20, 220, 80, 30, hwnd, (HMENU)BTN_SAVE, (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), NULL);

        // Tạo ListView
        hListView = CreateWindowEx(
            0, WC_LISTVIEW, NULL,
            WS_CHILD | WS_VISIBLE | LVS_REPORT, // Chế độ "Report View"
            220, 240, 640, 200, // Vị trí và kích thước
            hwnd, (HMENU)1, GetModuleHandle(NULL), NULL);
        SendMessage(hListView, LVM_SETBKCOLOR, 0, (LPARAM)RGB(220, 220, 220));

        // Thêm các cột và dữ liệu vào ListView
        LVCOLUMN lvColumn;
        lvColumn.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;

        lvColumn.pszText = (LPWSTR)L"Column 1";
        lvColumn.cx = 200;
        lvColumn.iSubItem = 0;
        ListView_InsertColumn(hListView, 0, &lvColumn);

        lvColumn.pszText = (LPWSTR)L"Column 2";
        lvColumn.cx = 200;
        lvColumn.iSubItem = 1;
        ListView_InsertColumn(hListView, 1, &lvColumn);

        lvColumn.pszText = (LPWSTR)L"Column 3";
        lvColumn.cx = 260;
        lvColumn.iSubItem = 2;
        ListView_InsertColumn(hListView, 2, &lvColumn);

        LVITEM lvItem;
        lvItem.mask = LVIF_TEXT;

        lvItem.iItem = 0;
        lvItem.iSubItem = 0;
        lvItem.pszText = (LPWSTR)L"Row 1, Col 1";
        ListView_InsertItem(hListView, &lvItem);
        ListView_SetItemText(hListView, 0, 1, (LPWSTR)L"Row 1, Col 2");
        ListView_SetItemText(hListView, 0, 2, (LPWSTR)L"Row 1, Col 3");

        lvItem.iItem = 1;
        lvItem.iSubItem = 0;
        lvItem.pszText = (LPWSTR)L"Row 2, Col 1";
        ListView_InsertItem(hListView, &lvItem);
        ListView_SetItemText(hListView, 1, 1, (LPWSTR)L"Row 2, Col 2");
        ListView_SetItemText(hListView, 1, 2, (LPWSTR)L"Row 2, Col 3");

        break;
    }

    case WM_COMMAND: {

        switch (LOWORD(wParam)) {
        case BTN_START: {
            disableEditing();
            disableUpdatingLog();
            if (type < 0) {
                DisplayContent(hwndEditDisplay, L"Please chose mode to start\n");
            } else if (!isStarted) {
                SetWindowText(hwndStart, L"Stop");
                SetWindowText(hwndEditDisplay, L"System Started...");
                isStarted = true;

                proxy = std::make_shared<ProxyServer>(type, LOCAL_PORT);
                std::cout << "Type " << type << "\n";
                std::thread p(ProxyServer::start, proxy.get());
                p.detach();
                

            } else {
                SetWindowText(hwndStart, L"Start");
                SetWindowText(hwndEditDisplay, L"System Stopped...");
                isStarted = false;
                proxy->stop(SIGINT);
            }
            
            break;
        }

        case BTN_BLACKLIST: {
            enableEditing();
            std::wifstream file(blacklistFilePath);
            std::wstring content((std::istreambuf_iterator<wchar_t>(file)), std::istreambuf_iterator<wchar_t>());
            file.close();
            SetWindowText(hwndEditDisplay, content.c_str());
            break;
        }

        case BTN_SAVE: {
            wchar_t buffer[65536];
            GetWindowText(hwndEditDisplay, buffer, 65536);
            SaveFile(blacklistFilePath, buffer);
            MessageBox(hwnd, L"Blacklist saved successfully!", L"Info", MB_OK | MB_ICONINFORMATION);

            disableUpdatingLog();
            break;
        }

        case BTN_HELP: {
            disableEditing();
            disableUpdatingLog();            

            SetWindowText(hwndEditDisplay, L"Help:\r\n- Start: Start/Stop system.\r\n- Choose Mode: Select mode.\r\n- Blacklist: Edit blocked items.\r\n- Log: View logs.");
            break;
        }

        case BTN_LOG: {
            disableEditing();
            enableUpdatingLog();
            std::wifstream file(logFilePath);
            std::wstring content((std::istreambuf_iterator<wchar_t>(file)), std::istreambuf_iterator<wchar_t>());
            DisplayContent(hwndEditDisplay, content);
            SendMessage(hwndEditDisplay, EM_LINESCROLL, 0, SendMessage(hwndEditDisplay, EM_GETLINECOUNT, 0, 0));
            isUpdatingLog = true;

            break;
        }

        case RADIO_MITM: {
            disableEditing();
            disableUpdatingLog();
            SendMessage(hwndRadioMITM, BM_SETCHECK, BST_CHECKED, 0);
            SendMessage(hwndRadioTransparent, BM_SETCHECK, BST_UNCHECKED, 0);

            SetWindowText(hwndEditDisplay, L"This is help for set up MITM proxy ....");
            type = MITM;
            break;
        }

        case RADIO_TRANSPARENT: {
            disableEditing();
            disableUpdatingLog();

            SendMessage(hwndRadioTransparent, BM_SETCHECK, BST_CHECKED, 0);
            SendMessage(hwndRadioMITM, BM_SETCHECK, BST_UNCHECKED, 0);
            type = Transparent;
            break;
        }
        }
        break;
    }

    case WM_DESTROY:
        PostQuitMessage(0);
        break;


    case WM_CTLCOLOREDIT: {
        // Kiểm tra xem điều khiển có phải là hwndEditDisplay không
        if ((HWND)lParam == hwndEditDisplay) {
            HDC hdc = (HDC)wParam;
            SetBkColor(hdc, RGB(192, 192, 192)); // Màu nền xám
            SetTextColor(hdc, RGB(0, 0, 0)); // Màu chữ đen
            return (LRESULT)hbrBackground; // Trả về cây cọ với màu nền xám
        }
        break;
    }

    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

