@echo off
echo [Span Engine] Generating Visual Studio Project...

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
pause