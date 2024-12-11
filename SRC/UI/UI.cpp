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

int GetLineHeight(HWND hwnd) {
    HDC hdc = GetDC(hwnd);
    HFONT hFont = (HFONT)SendMessage(hwnd, WM_GETFONT, 0, 0);
    HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);

    TEXTMETRIC tm;
    GetTextMetrics(hdc, &tm);

    SelectObject(hdc, hOldFont);
    ReleaseDC(hwnd, hdc);

    return tm.tmHeight;
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

PUI Window;

PUI::PUI() {
    
}

void PUI::init(LRESULT CALLBACK (*WindowProc)(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam), HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"MainWindow";
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    
    RegisterClass(&wc);

    // Tạo cửa sổ chính
    HWND hwnd = CreateWindowEx(
        0,
        L"MainWindow",
        L"Demo Application",
        (WS_OVERLAPPEDWINDOW | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME) & ~WS_MAXIMIZEBOX& ~WS_SIZEBOX & ~WS_THICKFRAME,
        CW_USEDEFAULT, CW_USEDEFAULT, WINDOW_WIDTH, WINDOW_HEIGHT,
        NULL, NULL, hInstance, NULL
    );

    if (!hwnd) return;

    ShowWindow(hwnd, nCmdShow);
}

void PUI::start() {
    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

void PUI::disableEditing() {
    SendMessage(hwndEdit, EM_SETREADONLY, TRUE, 0);
    EnableWindow(hwndSave, FALSE);
}

void PUI::enableEditing() {
    SendMessage(hwndEdit, EM_SETREADONLY, FALSE, 0);
    EnableWindow(hwndSave, TRUE);
}

void PUI::disableUpdatingLog() {
    isUpdatingLog = false;
}

void PUI::enableUpdatingLog() {
    isUpdatingLog = true;
}

bool PUI::IsAtBottomEdit() {
    int totalLines = SendMessage(hwndEdit, EM_GETLINECOUNT, 0, 0);
    int firstVisible = SendMessage(hwndEdit, EM_GETFIRSTVISIBLELINE, 0, 0);

    RECT rect;
    GetClientRect(hwndEdit, &rect);

    int visibleLines = rect.bottom / GetLineHeight(hwndEdit);
    return (firstVisible + visibleLines >= totalLines - 1);
}

bool PUI::IsAtBottomList() {
    // Lấy tổng số item trong ListView
    int totalItems = ListView_GetItemCount(hwndList);

    // Lấy chỉ số của item đầu tiên trong vùng hiển thị
    int firstVisible = SendMessage(hwndList, LVM_GETTOPINDEX, 0, 0);

    // Lấy kích thước của cửa sổ hiển thị
    RECT rect;
    GetClientRect(hwndList, &rect);

    // Khởi tạo RECT cho item
    RECT itemRect = { 0 };

    // Tính chiều cao của một item bằng cách lấy vị trí của item đầu tiên
    if (ListView_GetItemRect(hwndList, 0, &itemRect, LVIR_BOUNDS)) {
        int itemHeight = itemRect.bottom - itemRect.top;

        // Tính số dòng có thể hiển thị trong cửa sổ
        int visibleLines = rect.bottom / itemHeight;

        // Kiểm tra xem dòng cuối có nằm trong vùng hiển thị không
        return (firstVisible + visibleLines >= totalItems);
    }

    return false; // Trường hợp không thể lấy thông tin kích thước item
}

void PUI::AppendEdit(const std::wstring& content) {
    // Tạm thời tắt việc vẽ lại để tránh giật
    SendMessage(hwndEdit, WM_SETREDRAW, FALSE, 0);

    // Lưu trạng thái cuộn và vị trí chọn
    int curPos = SendMessage(hwndEdit, EM_GETFIRSTVISIBLELINE, 0, 0);
    bool atBottom = this->IsAtBottomEdit();
    int left, right;
    SendMessage(hwndEdit, EM_GETSEL, (WPARAM)&left, (LPARAM)&right);

    // Di chuyển con trỏ đến cuối và chèn nội dung mới
    int nLength = GetWindowTextLength(hwndEdit);
    SendMessage(hwndEdit, EM_SETSEL, (WPARAM)nLength, (LPARAM)nLength);
    SendMessage(hwndEdit, EM_REPLACESEL, (WPARAM)TRUE, (LPARAM)content.c_str());
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

void PUI::DisplayEdit(const std::wstring& content) {
    SetWindowText(hwndEdit, content.c_str());
}

void PUI::AppendList(const std::wstring state, const std::wstring host, const std::wstring port) {
    std::lock_guard<std::mutex> lock(mtx);
    // Tạm thời tắt việc vẽ lại để tránh giật
    SendMessage(hwndList, WM_SETREDRAW, FALSE, 0);

    // Kiểm tra số lượng dòng hiện tại
    int itemCount = ListView_GetItemCount(hwndList);
    int firstLine = SendMessage(hwndList, LVM_GETTOPINDEX, 0, 0);
    // Nếu số lượng dòng vượt quá maxLines, xóa dòng đầu tiên
    bool isDel = false;
    if (itemCount >= MAX_LINES) {
        ListView_DeleteItem(hwndList, 0);
        itemCount--; // Cập nhật lại số lượng dòng sau khi xóa
        isDel = true;
    }

    LVITEM lvItem;
    lvItem.mask = LVIF_TEXT;

    lvItem.iItem = SendMessage(hwndList, LVM_GETITEMCOUNT, 0, 0);
    lvItem.iSubItem = 0;
    lvItem.pszText = (LPWSTR)state.c_str();
    ListView_InsertItem(hwndList, &lvItem);
    ListView_SetItemText(hwndList, lvItem.iItem, 1, (LPWSTR)host.c_str());
    ListView_SetItemText(hwndList, lvItem.iItem, 2, (LPWSTR)port.c_str());

    // Kiểm tra nếu cuộn đã ở cuối, cuộn đến dòng cuối cùng
    if (this->IsAtBottomList()) {
        // Cuộn đến dòng cuối cùng
        SendMessage(hwndList, LVM_ENSUREVISIBLE, itemCount, TRUE);
    } else {
        SendMessage(hwndList, LVM_ENSUREVISIBLE, firstLine - isDel, TRUE);
    }

    // Bật lại việc vẽ lại và làm mới cửa sổ
    SendMessage(hwndList, WM_SETREDRAW, TRUE, 0);
    InvalidateRect(hwndList, NULL, TRUE);
}


// void PUI::StartLogUpdater() {
//     isUpdatingLog = true; // Bắt đầu cập nhật log

//     std::thread([&]() {
//         while (isUpdatingLog) {
//             // Đọc nội dung từ file log
//             std::wifstream file(logFilePath);
//             if (file.is_open()) {
//                 std::wstring content((std::istreambuf_iterator<wchar_t>(file)), std::istreambuf_iterator<wchar_t>());
//                 file.close();

//                 // Hiển thị nội dung log
//                 DisplayEdit(L"Hello\r\n");
//             }
//             // Đợi 1 giây trước lần cập nhật tiếp theo
//             std::this_thread::sleep_for(std::chrono::seconds(1));
//         }
//     }).detach(); // Tách luồng để chạy độc lập
// }

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_CREATE: {
        // Tạo các thành phần con
        Window.hwndStart = CreateWindow(L"BUTTON", L"Start", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
            20, 20, 80, 30, hwnd, (HMENU)BTN_START, (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), NULL);
        Window.hwndLog = CreateWindow(L"BUTTON", L"Log", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            120, 20, 80, 30, hwnd, (HMENU)BTN_LOG, (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), NULL);
        Window.hwndBlacklist = CreateWindow(L"BUTTON", L"Blacklist", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            20, 60, 80, 30, hwnd, (HMENU)BTN_BLACKLIST, (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), NULL);
        Window.hwndHelp = CreateWindow(L"BUTTON", L"Help", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            120, 60, 80, 30, hwnd, (HMENU)BTN_HELP, (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), NULL);
        Window.hwndGroupMode = CreateWindow(L"BUTTON", L"Choose Mode", WS_VISIBLE | WS_CHILD | BS_GROUPBOX,
            20, 110, 180, 100, hwnd, NULL, (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), NULL);
        Window.hwndRadioMITM = CreateWindow(L"BUTTON", L"MITM", WS_VISIBLE | WS_CHILD | BS_RADIOBUTTON,
            30, 140, 120, 20, hwnd, (HMENU)RADIO_MITM, (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), NULL);
        Window.hwndRadioTransparent = CreateWindow(L"BUTTON", L"Transparent", WS_VISIBLE | WS_CHILD | BS_RADIOBUTTON,
            30, 170, 120, 20, hwnd, (HMENU)RADIO_TRANSPARENT, (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), NULL);
        Window.hwndEdit = CreateWindowEx(WS_EX_STATICEDGE, L"EDIT", L"Content hello", WS_VISIBLE | WS_CHILD | WS_VSCROLL | ES_MULTILINE | ES_AUTOVSCROLL | WS_EX_WINDOWEDGE,
            220, 20, 640, 200, hwnd, (HMENU)EDIT_DISPLAY, (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), NULL);
        Window.hwndSave = CreateWindow(L"BUTTON", L"Save", WS_VISIBLE | WS_CHILD | WS_DISABLED | BS_PUSHBUTTON,
            20, 220, 80, 30, hwnd, (HMENU)BTN_SAVE, (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), NULL);

        // SendMessage(hwndEditDisplay, WM_SETFONT, (WPARAM)hFont, TRUE);
        Window.disableEditing();
        // Tạo ListView
        Window.hwndList = CreateWindowEx(
            WS_EX_STATICEDGE, WC_LISTVIEW, NULL,
            WS_CHILD | WS_VISIBLE | LVS_REPORT, // Chế độ "Report View"
            220, 240, 640, 200, // Vị trí và kích thước
            hwnd, (HMENU)1, GetModuleHandle(NULL), NULL);
        SendMessage(Window.hwndList, LVM_SETBKCOLOR, 0, (LPARAM)RGB(220, 220, 220));
        SendMessage(Window.hwndList, LVM_SETTEXTBKCOLOR, 0, (LPARAM)RGB(220, 220, 220));

        // Thêm các cột và dữ liệu vào ListView
        LVCOLUMN lvColumn;
        lvColumn.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;

        lvColumn.pszText = (LPWSTR)L"State";
        lvColumn.cx = 100;
        lvColumn.iSubItem = 0;
        ListView_InsertColumn(Window.hwndList, 0, &lvColumn);

        lvColumn.pszText = (LPWSTR)L"Host";
        lvColumn.cx = 400;
        lvColumn.iSubItem = 1;
        ListView_InsertColumn(Window.hwndList, 1, &lvColumn);

        lvColumn.pszText = (LPWSTR)L"Port";
        lvColumn.cx = 160;
        lvColumn.iSubItem = 2;
        ListView_InsertColumn(Window.hwndList, 2, &lvColumn);

        break;
    }

    case WM_COMMAND: {

        switch (LOWORD(wParam)) {
        case BTN_START: {
            Window.disableEditing();
            Window.disableUpdatingLog();

            if (Window.type < 0) {
                Window.DisplayEdit(L"Please chose mode to start");
            } else if (!Window.isStarted) {
                SetWindowText(Window.hwndStart, L"Stop");
                SetWindowText(Window.hwndEdit, L"System Started...");
                Window.isStarted = true;
                if (!Window.proxy || Window.proxy->getType() != Window.type) {
                    // std::cout << "Type = " << type << '\n';
                    // if (proxy)
                    //     std::cout << "Count = " << proxy.use_count() << '\n';
                    // proxy.reset();
                    // proxy.reset(new ProxyServer((Proxy)type, LOCAL_PORT));

                    delete Window.proxy;
                    Window.proxy = new ProxyServer((Proxy)Window.type, LOCAL_PORT);
                }
    
                std::thread p(ProxyServer::start, Window.proxy);
                p.detach();

            } else {
                SetWindowText(Window.hwndStart, L"Start");
                SetWindowText(Window.hwndEdit, L"System Stopped...");
                Window.isStarted = false;
                Window.proxy->stop(SIGINT);
            }
            
            break;
        }

        case BTN_BLACKLIST: {
            Window.enableEditing();
            Window.disableUpdatingLog();

            std::wifstream file(Window.blacklistFilePath);
            std::wstring content((std::istreambuf_iterator<wchar_t>(file)), std::istreambuf_iterator<wchar_t>());
            file.close();
            SetWindowText(Window.hwndEdit, content.c_str());
            break;
        }

        case BTN_SAVE: {
            wchar_t buffer[65536];
            GetWindowText(Window.hwndEdit, buffer, 65536);
            SaveFile(Window.blacklistFilePath, buffer);
            MessageBox(hwnd, L"Blacklist saved successfully!", L"Info", MB_OK | MB_ICONINFORMATION);
            break;
        }

        case BTN_HELP: {
            Window.disableEditing();
            Window.disableUpdatingLog();            

            SetWindowText(Window.hwndEdit, L"Help:\r\n- Start: Start/Stop system.\r\n- Choose Mode: Select mode.\r\n- Blacklist: Edit blocked items.\r\n- Log: View logs.");
            break;
        }

        case BTN_LOG: {
            Window.disableEditing();
            Window.enableUpdatingLog();
            std::wifstream file(Window.logFilePath);
            std::wstring content((std::istreambuf_iterator<wchar_t>(file)), std::istreambuf_iterator<wchar_t>());
            Window.DisplayEdit(content);
            SendMessage(Window.hwndEdit, EM_LINESCROLL, 0, SendMessage(Window.hwndEdit, EM_GETLINECOUNT, 0, 0));
            break;
        }

        case RADIO_MITM: {
            Window.disableEditing();
            Window.disableUpdatingLog();

            if (Window.isStarted && Window.type != MITM) {
                SetWindowText(Window.hwndEdit, L"Please stop before change mode");
                break;
            }
            SendMessage(Window.hwndRadioMITM, BM_SETCHECK, BST_CHECKED, 0);
            SendMessage(Window.hwndRadioTransparent, BM_SETCHECK, BST_UNCHECKED, 0);

            SetWindowText(Window.hwndEdit, L"This is help for set up MITM proxy ....");
            Window.type = MITM;
            break;
        }

        case RADIO_TRANSPARENT: {
            Window.disableEditing();
            Window.disableUpdatingLog();

            if (Window.isStarted && Window.type != Transparent) {
                SetWindowText(Window.hwndEdit, L"Please stop before change mode");
                break;
            }
            SendMessage(Window.hwndRadioTransparent, BM_SETCHECK, BST_CHECKED, 0);
            SendMessage(Window.hwndRadioMITM, BM_SETCHECK, BST_UNCHECKED, 0);
            Window.type = Transparent;
            break;
        }
        }
        break;
    }

    case WM_ACTIVATE:
        if (wParam == WA_INACTIVE) {
            // Lưu lại control đang có focus trước khi mất kích hoạt
            Window.hwndPrevFocus = GetFocus();
        } else if (wParam == WA_ACTIVE || wParam == WA_CLICKACTIVE) {
            // Khôi phục focus khi cửa sổ được kích hoạt
            if (Window.hwndPrevFocus && IsWindow(Window.hwndPrevFocus)) {
                SetFocus(Window.hwndPrevFocus);
                Window.hwndPrevFocus = NULL; // Reset biến sau khi focus đã khôi phục
            }
        }
        break;

    case WM_DESTROY:
        Window.proxy->stop(SIGINT);
        delete Window.proxy;
        Window.proxy = nullptr;
        PostQuitMessage(0);
        break;


    // case WM_CTLCOLOREDIT: {
    //     // Kiểm tra xem điều khiển có phải là hwndEditDisplay không
    //     if ((HWND)lParam == Window.hwndEdit) {
    //         HDC hdc = (HDC)wParam;
    //         SetBkColor(hdc, RGB(220, 220, 220)); // Màu nền xám
    //         SetTextColor(hdc, RGB(0, 0, 0)); // Màu chữ đen
    //         return (LRESULT)Window.hbrBackground; // Trả về cây cọ với màu nền xám
    //     }
    //     break;
    // }

    case WM_CTLCOLOREDIT: {
        HDC hdcEdit = (HDC)wParam;
        if ((HWND)lParam == Window.hwndEdit) {
            // Nếu có thể chỉnh sửa
            if (!(GetWindowLong(Window.hwndEdit, GWL_STYLE) & ES_READONLY)) {
                SetBkColor(hdcEdit, RGB(220, 220, 220)); // Màu nền khi có thể chỉnh sửa
                SetTextColor(hdcEdit, RGB(0, 0, 0));     // Màu chữ (đen)
            }
            return (LRESULT)Window.hbrBackground;
        }
        break;
    }

    case WM_CTLCOLORSTATIC: {
        HDC hdcStatic = (HDC)wParam;
        if ((HWND)lParam == Window.hwndEdit) {
            // Nếu không thể chỉnh sửa (read-only)
            if (GetWindowLong(Window.hwndEdit, GWL_STYLE) & ES_READONLY) {
                SetBkColor(hdcStatic, RGB(220, 220, 220)); // Màu nền khi không chỉnh sửa
                SetTextColor(hdcStatic, RGB(0, 0, 0)); // Màu chữ (xám nhạt)
            }
            return (LRESULT)Window.hbrBackground;
        }
        break;
    }

    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

