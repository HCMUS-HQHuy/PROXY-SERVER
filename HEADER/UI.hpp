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

    template <typename... Args>
    void PUI::AppendList(Args&&... args) {
        std::lock_guard<std::mutex> lock(mtx);

        // Tạm thời tắt việc vẽ lại để tránh giật
        SendMessage(hwndList, WM_SETREDRAW, FALSE, 0);

        // Kiểm tra số lượng dòng hiện tại
        int itemCount = ListView_GetItemCount(hwndList);
        int firstLine = SendMessage(hwndList, LVM_GETTOPINDEX, 0, 0);

        // Nếu số lượng dòng vượt quá MAX_LINES, xóa dòng đầu tiên
        bool isDel = false;
        if (itemCount >= MAX_LINES) {
            ListView_DeleteItem(hwndList, 0);
            itemCount--; // Cập nhật lại số lượng dòng sau khi xóa
            isDel = true;
        }

        // Tạo danh sách các đối số
        std::vector<std::wstring> rowData = {std::forward<Args>(args)...};

        // Tạo một hàng mới
        LVITEM lvItem = {};
        lvItem.mask = LVIF_TEXT;
        lvItem.iItem = itemCount; // Hàng mới sẽ nằm ở cuối
        lvItem.iSubItem = 0; // Luôn bắt đầu với cột đầu tiên
        lvItem.pszText = (LPWSTR)rowData[0].c_str(); // Dữ liệu cho cột đầu tiên
        ListView_InsertItem(hwndList, &lvItem);

        // Thêm dữ liệu vào các cột còn lại
        int columnCount = Header_GetItemCount(ListView_GetHeader(hwndList));
        for (size_t i = 1; i < rowData.size() && i < (size_t)columnCount; ++i) {
            ListView_SetItemText(hwndList, lvItem.iItem, i, (LPWSTR)rowData[i].c_str());
        }

        // Kiểm tra nếu cuộn đã ở cuối, cuộn đến dòng cuối cùng
        if (this->IsAtBottomList()) {
            SendMessage(hwndList, LVM_ENSUREVISIBLE, itemCount, TRUE);
        } else {
            SendMessage(hwndList, LVM_ENSUREVISIBLE, firstLine - isDel, TRUE);
        }

        // Bật lại việc vẽ lại và làm mới cửa sổ
        SendMessage(hwndList, WM_SETREDRAW, TRUE, 0);
        InvalidateRect(hwndList, NULL, TRUE);
    }
};

int GetLineHeight(HWND hwnd);

extern PUI Window;
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

#endif