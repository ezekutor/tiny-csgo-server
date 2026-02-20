set HL2SDK_DIR=""
set ASIO_DIR=""

if %HL2SDK_DIR%==""  (
    echo Warning: Please set hl2sdk-cs2 path in build.bat before running.
    exit /b
)

if %ASIO_DIR%==""  (
    echo Warning: Please set asio path in build.bat before running.
    exit /b
)

cmake -G "Visual Studio 17 2022" -A x64 -B "vs2022" -DHL2SDK_DIR:PATH=%HL2SDK_DIR% -DASIO_SRC:PATH=%ASIO_DIR%
cmake --build vs2022 --config Release
pause
