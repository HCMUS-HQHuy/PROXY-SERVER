@echo off
setlocal
setlocal enabledelayedexpansion

set src_dir=./SRC
set obj_dir=./BIN
set compiler=g++
set flags=-c
set libs=-lgdi32 -luser32 -lws2_32 -liphlpapi
set output=./BIN/demo.exe

if not exist "%obj_dir%" mkdir "%obj_dir%"

for %%f in (%src_dir%/*.cpp) do (
    set src_file=%src_dir%/%%f
    set obj_file=%obj_dir%/%%~nf.o

    if not exist "!obj_file!" (
        set compile=true
    ) else (
        for %%i in ("!src_file!") do for %%j in ("!obj_file!") do (
            if %%~ti GTR %%~tj (
                set compile=true
            ) 
        )
    )
    :: Compile if needed
    if defined compile (
        echo Compiling %%f...
        %compiler% %flags% "!src_file!" -o "!obj_file!" %libs%
        if errorlevel 1 exit /b 1
        set compile=
        set compliedFile=!compliedFile! %%~nf
    )
    :: Add object file to the list
    set obj_files=!obj_files! !obj_file!

) 

echo compliedFile:%compliedFile%
echo NameExeFile : %output%

%compiler% -o %output% %obj_files% %libs%

if errorlevel 1 (
    echo Build failed with errors.
    exit /b %errorlevel%
) else (
    echo Build succeeded. 
    @REM Code is running...
    @REM echo ----------------***----------------
)

:: Run the executable
@REM %NameExeFile%
@REM set return_code=%errorlevel%

:: Display the return code from the executable
@REM echo ----------------***----------------
@REM echo Program returned: %return_code%

exit /b @REM %return_code%
