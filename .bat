@echo off
setlocal enabledelayedexpansion

set src_dir=./SRC
set obj_dir=./BIN
set compiler=g++
set flags=-c -Wall -Wextra -Wcast-align -Wwrite-strings -Waggregate-return -O2
set libs=-lgdi32 -luser32 -lws2_32 -liphlpapi  -I"C:/Program Files/OpenSSL-Win64/include" -L"C:\Program Files\OpenSSL-Win64\lib\VC\x64\MT" -lssl -lcrypto -fpermissive
set output=./BIN/demo.exe

if not exist "%obj_dir%" mkdir "%obj_dir%"


:: Xóa danh sách file object để đảm bảo không bị chồng lẫn
set obj_files=

:: Duyệt qua tất cả các file .cpp trong src_dir và các thư mục con
for /R %src_dir% %%f in (*.cpp) do (
    :: Xác định đường dẫn file nguồn và file đối tượng tương ứng
    set src_file=%%f
    set rel_path=%%~pf
    set rel_path=!rel_path:%src_dir%=!
    set obj_file=%obj_dir%!rel_path!%%~nf.o

    :: Tạo các thư mục trong obj_dir nếu chưa tồn tại
    if not exist "!obj_file!" (
        mkdir "%obj_dir%!rel_path!" >nul 2>&1
        set compile=true
    ) else (
        :: Kiểm tra thời gian cập nhật để xác định có cần biên dịch lại hay không
        for %%i in ("!src_file!") do for %%j in ("!obj_file!") do (
            if %%~ti GTR %%~tj (
                set compile=true
            ) 
        )
    )

    :: Biên dịch nếu cần thiết
    if defined compile (
        echo Compiling %%f...
        %compiler% %flags% "!src_file!" -o "!obj_file!" %libs%
        if errorlevel 1 exit /b 1
        set compile=
    )

    :: Thêm file đối tượng vào danh sách
    set obj_files=!obj_files! !obj_file!
)

:: Liên kết các file đối tượng thành file thực thi
echo Linking objects to executable...
%compiler% !obj_files! -o %output% %libs%
if errorlevel 1 exit /b 1

echo Built successfully.

exit /b