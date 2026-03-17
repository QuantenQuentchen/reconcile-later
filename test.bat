@echo off

set FOUND=0
echo Searching for Visual Studio environment...
echo Why ?
echo Because Microsoft in their infinite Billion Dollar Wisdom decided to not allow us to use the compiler just as is.

rem Try VS 2017 x64, then x86
if %FOUND%==0 call "C:\Program Files (x86)\Microsoft Visual Studio\2026\Community\VC\Auxiliary\Build\vcvars64.bat" && set FOUND=1
if %FOUND%==0 call "C:\Program Files (x86)\Microsoft Visual Studio\2026\Community\VC\Auxiliary\Build\vcvars32.bat" && set FOUND=1

if %FOUND%==0 call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" && set FOUND=1

rem Try VS 2019 x64, then x86
if %FOUND%==0 call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat" && set FOUND=1
if %FOUND%==0 call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars32.bat" && set FOUND=1

rem Try VS 2022 x64, then x86
if %FOUND%==0 call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" && set FOUND=1
if %FOUND%==0 call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars32.bat" && set FOUND=1

rem Try VS 2017 x64, then x86
if %FOUND%==0 call "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvars64.bat" && set FOUND=1
if %FOUND%==0 call "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvars32.bat" && set FOUND=1

if %FOUND%==0 (
    echo Could not find a working vcvars batch file!
    exit /b 1
)


rem Check for updates
echo Checking for Updates!
echo ...
git pull origin master > gitpull.log
type gitpull.log
findstr /C:"Already up to date" gitpull.log >nul
if errorlevel 1 (
    echo Repository was updated. Recalling script to apply changes.
    del gitpull.log
    call "%~f0"
    exit /b
)
del gitpull.log
echo Updates checked!

rem Build and run
echo Recompiling, don't know if we have to. But we do it anyway!
if not exist build (
    mkdir build
)
cd build
cmake .. -DCMAKE_POLICY_VERSION_MINIMUM=3.5
echo "Build files generated!"
echo "Compiling, with all your cores, you see stutters, that's the reason, you want it to not stutter, edit the script."
cmake --build . --parallel
echo Recompiling done!
echo Starting the program!
echo If something doesn't seem to work, try nuking .\build, and rerunning this script before complaining
.\Debug\N_1.exe
