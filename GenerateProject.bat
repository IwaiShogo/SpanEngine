@echo off
:: バッチファイルの場所にカレントディレクトリを移動 (管理者実行時などのパスずれ防止)
pushd %~dp0

echo ============================================================
echo [1/2] Generating SpanEngine.h ...
echo ============================================================

rem PowerShellスクリプトの実行 (ExecutionPolicyを一時的にBypass)
powershell -NoProfile -ExecutionPolicy Bypass -File "Tools/HeaderGenerator/GenerateEngineHeader.ps1"

if %ERRORLEVEL% NEQ 0 (
	echo [Error] Header generation failed!
	pause
	exit /b %ERRORLEVEL%
)

echo.
echo ============================================================
echo [2/2] Generating Visual Studio Project ...
echo ============================================================

:: Buildフォルダがない場合は作成
if not exist "Build" mkdir "Build"

:: CMakeを実行してソリューションファイルを生成
:: -S .  -> ソースコードはここ（ルート）にあるよ
:: -B Build -> 生成物はBuildフォルダに入れてね
cmake -S . -B Build

if %ERRORLEVEL% NEQ 0 (
    echo [Error] CMake generation failed!
    pause
    exit /b %ERRORLEVEL%
)

echo.
echo [Success] Solution file generated in 'Build' folder.
echo You can now open SpanEngine.sln

:: 元のディレクトリに戻す
popd
pause
