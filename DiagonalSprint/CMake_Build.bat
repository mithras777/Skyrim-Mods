@echo off
setlocal
pushd "%~dp0"
if errorlevel 1 exit /b 1
cmake --preset ng
set "result=%errorlevel%"
if not "%result%"=="0" goto finished
cmake --build --preset ng-release
set "result=%errorlevel%"
:finished
popd
exit /b %result%
