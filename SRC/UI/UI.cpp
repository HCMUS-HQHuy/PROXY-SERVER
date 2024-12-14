#include "../../HEADER/UI.hpp"
#include <fstream>
#include <thread>
#include <chrono>
#include <atomic>
#include <bits/shared_ptr.h>
#include "../../HEADER/ProxyServer.hpp"
#include "../../HEADER/Setting.hpp"
#include "../../HEADER/BlackList.hpp"
#include <string.h>
#include <commctrl.h>
#include <gdiplus.h>
#include <cwchar>  
#include <cstring>
#include <richedit.h>

PUI Window;

const wchar_t* PrevMitmMsg = 
    L"Follow the instructions to complete the setup for MITM mode.\r\n"
    "- Import Certificate into system:\r\n"
    "  + Find the root.crt file in the CERTIFICATE folder of your MyProxy installation.\r\n"
    "  + Open \"Manage user certificates\" on your operating system.\r\n"
    "  + Import root.crt file into the \"Trusted Root Certificate Authorities\" store.\r\n"

    "- Configure Proxy Settings\r\n"
    "  + Open the Proxy Settings on your computer.\r\n"
    "  + Enable Manual Proxy Setup and enter the following details:\r\n"
    "    + Host: ";
    
const wchar_t* PostMitmMsg = 
    L"\r\n"
    "    + Port: 8080\r\n"
    "- Start and run...";

const wchar_t* PrevTransMsg = 
    L"Follow the instructions to complete the setup for Transparent mode.\r\n"
    "- Configure Proxy Settings\r\n"
    "  + Open the Proxy Settings on your computer.\r\n"
    "  + Enable Manual Proxy Setup and enter the following details:\r\n"
    "    + Host: ";

const wchar_t* PostTransMsg =
    L"\r\n"
    "    + Port: 8080\r\n"
    "- Start and run...";


using namespace Gdiplus;

void InitGDIPlus() {
    GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
}

// Hàm xử lý vẽ ảnh logo
void DrawLogo(HWND hwnd, HDC hdc) {
    Graphics graphics(hdc);
    Image image(L"./ASSETS/Proxy_logo.png");
    RECT rect;
    GetClientRect(hwnd, &rect);
    graphics.DrawImage(&image, rect.left, rect.top + 254 - 1, 220, 220); // Hiển thị ảnh với kích thước 200x200
}

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

bool SaveFile(const std::string& filePath, const std::wstring& content) {
    std::wofstream file(filePath, std::ios::trunc);
    if (file.is_open()) {
        file << content;
        file.close();
        return true;
    }

    return false;
}



PUI::PUI() {
    
}

void PUI::init(LRESULT CALLBACK (*WindowProc)(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam), HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    InitGDIPlus();
    WNDCLASSEX wc = {};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"MainWindow";
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.hIcon = (HICON)LoadImage(
        NULL,
        L"./ASSETS/Proxy_logo_white_background.ico", // Đường dẫn đến file ico
        IMAGE_ICON,
        32, 32,            // Kích thước icon
        LR_LOADFROMFILE    // Tải từ file
    );
    wc.hIconSm = (HICON)LoadImage(
        NULL,
        L"./ASSETS/Proxy_logo.ico",
        IMAGE_ICON,
        16, 16,
        LR_LOADFROMFILE
    );

    
    RegisterClassEx(&wc);

    // Tạo cửa sổ chính
    HWND hwnd = CreateWindowEx(
        0,
        L"MainWindow",
        L"Proxy Server",
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

// bool PUI::IsAtBottomEdit() {
//     int totalLines = SendMessage(hwndEdit, EM_GETLINECOUNT, 0, 0);
//     int firstVisible = SendMessage(hwndEdit, EM_GETFIRSTVISIBLELINE, 0, 0);

//     RECT rect;
//     GetClientRect(hwndEdit, &rect);

//     int visibleLines = rect.bottom / GetLineHeight(hwndEdit);
//     return (firstVisible + visibleLines >= totalLines - 1);
// }

bool PUI::IsAtBottomList() {
    // Lấy tổng số item trong ListView
    int totalItems = ListView_GetItemCount(hwndList);

    // Lấy chỉ số của item đầu tiên trong vùng hiển thị
    int firstVisible = SendMessage(hwndList, LVM_GETTOPINDEX, 0, 0);

    // Lấy kích thước của cửa sổ hiển thị
    RECT rect;
    GetClientRect(hwndList, &rect);

    // Khởi tạo RECT cho item
    RECT itemRect;

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

bool PUI::IsAtBottomEdit() {
    // Lấy tổng số dòng và dòng đầu tiên nhìn thấy
    DWORD dwLineCount = SendMessage(hwndEdit, EM_GETLINECOUNT, 0, 0);
    DWORD dwFirstVisible = SendMessage(hwndEdit, EM_GETFIRSTVISIBLELINE, 0, 0);

    RECT rect;
    GetClientRect(hwndEdit, &rect);

    // Tính số dòng hiển thị
    int visibleLines = rect.bottom / GetLineHeight(hwndEdit);

    // Kiểm tra xem có đang ở cuối hay không
    return (dwFirstVisible + visibleLines >= dwLineCount - 1);
}

void PUI::ScrolltoEnd() {
    int firstVisible = SendMessage(hwndEdit, EM_GETFIRSTVISIBLELINE, 0, 0);
    int totalLines = SendMessage(hwndEdit, EM_GETLINECOUNT, 0, 0);

    // Calculate visible lines in RichEdit (assumes multi-line mode)
    RECT rc;
    GetClientRect(hwndEdit, &rc);
    int visibleLines = rc.bottom / GetLineHeight(hwndEdit);
    int linesToScroll = totalLines - visibleLines - firstVisible;
    if (linesToScroll > 0) {
        SendMessage(hwndEdit, EM_LINESCROLL, 0, linesToScroll);
    }

}

void PUI::AppendEdit(const std::wstring& content) {
    if (!isUpdatingLog) return;
    std::lock_guard<std::mutex> lock(mtx);

    // Tạm thời tắt việc vẽ lại để tránh giật
    SendMessage(hwndEdit, WM_SETREDRAW, FALSE, 0);

    // Lưu trạng thái cuộn và vị trí chọn
    DWORD curPos = SendMessage(hwndEdit, EM_GETFIRSTVISIBLELINE, 0, 0);
    bool atBottom = this->IsAtBottomEdit();
    int left, right;
    SendMessage(hwndEdit, EM_GETSEL, (WPARAM)&left, (LPARAM)&right);

    // Di chuyển con trỏ đến cuối và chèn nội dung mới
    DWORD nLength = GetWindowTextLength(hwndEdit);
    SendMessage(hwndEdit, EM_SETSEL, (WPARAM)nLength, (LPARAM)nLength);
    SendMessage(hwndEdit, EM_REPLACESEL, (WPARAM)TRUE, (LPARAM)content.c_str());
    SendMessage(hwndEdit, EM_SETSEL, left, right);

    // Điều chỉnh lại vị trí cuộn
    if (atBottom) {
        ScrolltoEnd();
    }

    // Bật lại việc vẽ lại và làm mới cửa sổ
    SendMessage(hwndEdit, WM_SETREDRAW, TRUE, 0);
    InvalidateRect(hwndEdit, NULL, TRUE);
    UpdateWindow(hwndEdit); // Cập nhật ngay lập tức
}

void PUI::DisplayEdit(const std::wstring& content) {
    // Nếu bạn chỉ muốn thay thế toàn bộ nội dung, có thể sử dụng SetWindowText
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


LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        DrawLogo(hwnd, hdc);

        // Bút và chổi vẽ
        HPEN hPen = CreatePen(PS_SOLID, 1, RGB(120, 120, 120)); // Bút viền màu xanh
        HBRUSH hBrush = (HBRUSH)GetStockObject(NULL_BRUSH); // Không tô đầy
        SelectObject(hdc, hPen);
        SelectObject(hdc, hBrush);

        // Vẽ khung cho hwndEdit
        RECT rect;
        GetWindowRect(Window.hwndEdit, &rect);
        MapWindowPoints(HWND_DESKTOP, hwnd, (LPPOINT)&rect, 2); // Chuyển tọa độ màn hình sang cửa sổ
        Rectangle(hdc, rect.left - 1, rect.top - 1, rect.right + 1, rect.bottom + 1);

        GetWindowRect(Window.hwndTitleMessage, &rect);
        MapWindowPoints(HWND_DESKTOP, hwnd, (LPPOINT)&rect, 2); // Chuyển tọa độ màn hình sang cửa sổ
        Rectangle(hdc, rect.left - 1, rect.top - 1, rect.right + 1, rect.bottom + 1);

        GetWindowRect(Window.hwndBlacklist, &rect);
        MapWindowPoints(HWND_DESKTOP, hwnd, (LPPOINT)&rect, 2); // Chuyển tọa độ màn hình sang cửa sổ
        Rectangle(hdc, rect.left - 1, rect.top - 1, rect.right + 1, rect.bottom + 1);

        GetWindowRect(Window.hwndTitleBlacklist, &rect);
        MapWindowPoints(HWND_DESKTOP, hwnd, (LPPOINT)&rect, 2); // Chuyển tọa độ màn hình sang cửa sổ
        Rectangle(hdc, rect.left - 1, rect.top - 1, rect.right + 1, rect.bottom + 1);
        
        GetWindowRect(Window.hwndList, &rect);
        MapWindowPoints(HWND_DESKTOP, hwnd, (LPPOINT)&rect, 2);
        Rectangle(hdc, rect.left - 1, rect.top - 1, rect.right + 1, rect.bottom + 1);

        // Giải phóng tài nguyên
        DeleteObject(hPen);

        EndPaint(hwnd, &ps);
        break;
    }
    case WM_CREATE: {
        LoadLibrary(TEXT("Msftedit.dll"));
        // Tạo các thành phần con
        Window.hwndStart = CreateWindow(L"BUTTON", L"Start", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
            20, 20, 180, 50, hwnd, (HMENU)BTN_START, (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), NULL);
        Window.hwndLog = CreateWindow(L"BUTTON", L"Log", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            20, 90, 80, 30, hwnd, (HMENU)BTN_LOG, (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), NULL);
        Window.hwndHelp = CreateWindow(L"BUTTON", L"Help", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            120, 90, 80, 30, hwnd, (HMENU)BTN_HELP, (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), NULL);
        Window.hwndGroupMode = CreateWindow(L"BUTTON", L" Mode ", WS_VISIBLE | WS_CHILD | BS_GROUPBOX,
            20, 140, 180, 100 + 4, hwnd, NULL, (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), NULL);
        Window.hwndRadioMITM = CreateWindow(L"BUTTON", L"MITM", WS_VISIBLE | WS_CHILD | BS_RADIOBUTTON,
            30, 170 + 1, 120, 20, hwnd, (HMENU)RADIO_MITM, (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), NULL);
        Window.hwndRadioTransparent = CreateWindow(L"BUTTON", L"Transparent", WS_VISIBLE | WS_CHILD | BS_RADIOBUTTON,
            30, 200 + 3, 120, 20, hwnd, (HMENU)RADIO_TRANSPARENT, (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), NULL);

        // Tạo hwndEdit
        Window.hwndEdit = CreateWindow(MSFTEDIT_CLASS, L"Content hello", WS_VISIBLE | WS_CHILD | ES_MULTILINE | ES_AUTOVSCROLL | WS_VSCROLL | ES_MULTILINE,
            220, 40, 440, 200 + 4, hwnd, (HMENU)EDIT_DISPLAY, (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), NULL);
        Window.hwndTitleMessage = CreateWindow( L"STATIC", L"Message", // Văn bản hiển thị
                                                WS_VISIBLE | WS_CHILD | SS_CENTER,
                                                220, 20, 440, 20 - 1, // Vị trí và kích thước
                                                hwnd, (HMENU)TITLE_BLACKLIST, NULL, NULL);
        // Tạo hwndBlacklist
        Window.hwndBlacklist = CreateWindow(MSFTEDIT_CLASS, L"Blacklist", WS_VISIBLE | WS_CHILD | ES_MULTILINE | ES_AUTOVSCROLL | ES_LEFT | WS_VSCROLL | ES_MULTILINE,
            670, 40, 200, 170, hwnd, (HMENU)EDIT_BLACKLIST, (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), NULL);
        Window.hwndTitleBlacklist = CreateWindow(L"STATIC", L"Blacklist", // Văn bản hiển thị
                                                WS_VISIBLE | WS_CHILD | SS_CENTER,
                                                670, 20, 200, 20 - 1, // Vị trí và kích thước
                                                hwnd, (HMENU)TITLE_BLACKLIST, NULL, NULL);
        // Tạo nút Save
        Window.hwndSave = CreateWindow(L"BUTTON", L"Save", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            670 - 1, 210 + 4, 200 + 2, 30, hwnd, (HMENU)BTN_SAVE, (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), NULL);

        // RECT rc;
        // SendMessage(Window.hwndEdit, EM_GETRECT, 0, (LPARAM)&rc);  // Lấy vùng hiển thị hiện tại
        // rc.top += 2;  // Cập nhật lề trên với giá trị margin mới
        
        // // Gửi thông điệp EM_SETRECT để cập nhật lề trên
        // SendMessage(Window.hwndEdit, EM_SETRECT, 0, (LPARAM)&rc);
        SendMessage(Window.hwndEdit, EM_SETREADONLY, TRUE, 0);
        SendMessage(Window.hwndEdit, EM_SETBKGNDCOLOR, 0, (LPARAM)RGB(220, 220, 220));
        SendMessage(Window.hwndBlacklist, EM_SETBKGNDCOLOR, 0, (LPARAM)RGB(220, 220, 220));
        SendMessage(Window.hwndEdit, EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, MAKELPARAM(4, 4));
        SendMessage(Window.hwndBlacklist, EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, MAKELPARAM(4, 4));
        EnableWindow(Window.hwndSave, TRUE);
        std::wifstream file(Window.blacklistFilePath);
        std::wstring content((std::istreambuf_iterator<wchar_t>(file)), std::istreambuf_iterator<wchar_t>());
        file.close();
        SetWindowText(Window.hwndBlacklist, content.c_str());

        // Tạo ListView
        Window.hwndList = CreateWindow(
            WC_LISTVIEW, NULL,
            WS_CHILD | WS_VISIBLE | LVS_REPORT | WS_VSCROLL | LVS_EX_FLATSB, // Chế độ "Report View"
            220, 260 + 4, 650, 200 - 2, // Vị trí và kích thước
            hwnd, (HMENU)1, GetModuleHandle(NULL), NULL);
        SendMessage(Window.hwndList, LVM_SETBKCOLOR, 0, (LPARAM)RGB(220, 220, 220));
        SendMessage(Window.hwndList, LVM_SETTEXTBKCOLOR, 0, (LPARAM)RGB(220, 220, 220));

        // Thêm các cột và dữ liệu vào ListView
        LVCOLUMN lvColumn;
        ZeroMemory(&lvColumn, sizeof(lvColumn)); // Reset the structure
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
        lvColumn.cx = 133;
        lvColumn.iSubItem = 2;
        ListView_InsertColumn(Window.hwndList, 2, &lvColumn);

        break;
    }

    case WM_COMMAND: {

        switch (LOWORD(wParam)) {
        case BTN_START: {
            Window.disableUpdatingLog();

            if (Window.type < 0) {
                Window.DisplayEdit(L"Please select a mode before starting the proxy server.");
            } else if (!Window.isStarted) {

                SetWindowText(Window.hwndStart, L"Stop");
                std::string ip = NetworkManager::getIPv4();
                size_t len = std::mbstowcs(nullptr, ip.c_str(), 0) + 1;  // Tính toán độ dài cần thiết
                wchar_t* long_ip = new wchar_t[len];
                std::mbstowcs(long_ip, ip.c_str(), len);
                Window.DisplayEdit(std::wstring(L"Proxy server is listening on IPv4: ") +
                    std::wstring(long_ip) + std::wstring(L"\r\nPort: 8080...\r\n(local)\r\nUse loopback IPaddress: 127.0.0.1\r\nPort: 8080..."));
                delete[] long_ip;
                Window.isStarted = true;
                if (!Window.proxy || Window.proxy->getType() != Window.type) {
                    delete Window.proxy;
                    Window.proxy = new ProxyServer((Proxy)Window.type, LOCAL_PORT);
                }

                std::thread p(ProxyServer::start, Window.proxy);
                p.detach();

            } else {
                SetWindowText(Window.hwndStart, L"Start");
                SetWindowText(Window.hwndEdit, L"Proxy server stopped.");
                Window.isStarted = false;
                Window.proxy->stop(SIGINT);
            }
            
            break;
        }

        case BTN_SAVE: {
            wchar_t buffer[65536];
            GetWindowText(Window.hwndBlacklist, buffer, 65536);
            if (!SaveFile(Window.blacklistFilePath, buffer))
                MessageBox(hwnd, L"Failed to save blacklist file.", L"Error", MB_OK | MB_ICONERROR);
            // MessageBox(hwnd, L"Blacklist saved successfully!", L"Info", MB_OK | MB_ICONINFORMATION);
            else if (blackList.reload()) {
                MessageBox(hwnd, L"Blacklist saved successfully!", L"Info", MB_OK | MB_ICONINFORMATION);
            } else {
                MessageBox(hwnd, L"Failed to reload blacklist file.", L"Error", MB_OK | MB_ICONERROR);
            }
            break;
        }

        case BTN_HELP: {
            Window.disableUpdatingLog();            

            SetWindowText(Window.hwndEdit, L"Help:\r\n- Start: Start/Stop system.\r\n- Choose Mode: Select mode.\r\n- Blacklist: Edit blocked items.\r\n- Log: View logs.");
            break;
        }

        case BTN_LOG: {
            Window.enableUpdatingLog();
            std::wifstream file(Window.logFilePath);
            std::wstring content((std::istreambuf_iterator<wchar_t>(file)), std::istreambuf_iterator<wchar_t>());
            Window.DisplayEdit(content);
            // SendMessage(Window.hwndEdit, EM_LINESCROLL, 0, SendMessage(Window.hwndEdit, EM_GETLINECOUNT, 0, 0));
            Window.ScrolltoEnd();
            break;
        }

        case RADIO_MITM: {
            Window.disableUpdatingLog();

            if (Window.isStarted && Window.type != MITM) {
                SetWindowText(Window.hwndEdit, L"Please stop the proxy server before changing the mode.");
                break;
            }
            SendMessage(Window.hwndRadioMITM, BM_SETCHECK, BST_CHECKED, 0);
            SendMessage(Window.hwndRadioTransparent, BM_SETCHECK, BST_UNCHECKED, 0);

            std::string ip = NetworkManager::getIPv4();
            size_t len = std::mbstowcs(nullptr, ip.c_str(), 0) + 1;  // Tính toán độ dài cần thiết
            wchar_t* long_ip = new wchar_t[len];
            std::mbstowcs(long_ip, ip.c_str(), len);
            Window.DisplayEdit(std::wstring(PrevMitmMsg) +
            std::wstring(long_ip) + std::wstring(PostMitmMsg));
            delete[] long_ip;
            Window.type = MITM;
            break;
        }

        case RADIO_TRANSPARENT: {
            Window.disableUpdatingLog();

            if (Window.isStarted && Window.type != Transparent) {
                SetWindowText(Window.hwndEdit, L"Please stop the proxy server before changing the mode.");
                break;
            }
            SendMessage(Window.hwndRadioTransparent, BM_SETCHECK, BST_CHECKED, 0);
            SendMessage(Window.hwndRadioMITM, BM_SETCHECK, BST_UNCHECKED, 0);
            std::string ip = NetworkManager::getIPv4();
            size_t len = std::mbstowcs(nullptr, ip.c_str(), 0) + 1;  // Tính toán độ dài cần thiết
            wchar_t* long_ip = new wchar_t[len];
            std::mbstowcs(long_ip, ip.c_str(), len);
            Window.DisplayEdit(std::wstring(PrevTransMsg) +
            std::wstring(long_ip) + std::wstring(PostTransMsg));
            delete[] long_ip;
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

    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}
