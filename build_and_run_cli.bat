@echo off
title Metro Route Finder - Console CLI
echo ========================================================
echo   Compiling Metro Route Finder Console CLI (main.cpp)
echo ========================================================
g++ -std=c++14 main.cpp Graph.cpp Metro.cpp -o MetroApp.exe
if %errorlevel% neq 0 (
    echo.
    echo [ERROR] Compilation failed! Please check if g++ is configured in PATH.
    pause
    exit /b %errorlevel%
)
echo.
echo [SUCCESS] Compilation succeeded: MetroApp.exe created.
echo.
echo Launching Metro Route Finder Console CLI...
echo.
MetroApp.exe
pause
