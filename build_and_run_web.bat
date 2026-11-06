@echo off
title Metro Route Finder - Web Server
echo ========================================================
echo   Compiling Metro Route Finder C++ HTTP Web Server
echo ========================================================
g++ -std=c++14 server.cpp Graph.cpp Metro.cpp -o metro_server.exe -lws2_32
if %errorlevel% neq 0 (
    echo.
    echo [ERROR] Compilation failed! Please check if g++ is configured in PATH.
    pause
    exit /b %errorlevel%
)
echo.
echo [SUCCESS] Compilation succeeded: metro_server.exe created.
echo.
echo Opening http://127.0.0.1:8080 in your default web browser...
start http://127.0.0.1:8080
echo Starting native C++ server on port 8080 (Press Ctrl+C to stop)...
echo.
metro_server.exe
pause
