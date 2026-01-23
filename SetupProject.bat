@echo off
echo [Span Engine] Initializing Directory Structure...

:: --- Root Directories ---
mkdir Bin
mkdir Build
mkdir Tools
mkdir Tools\ReflectionParser

:: --- Engine Directories ---
mkdir Engine
mkdir Engine\Source
mkdir Engine\Source\Core
mkdir Engine\Source\Runtime
mkdir Engine\Source\Editor
mkdir Engine\Source\Developer
mkdir Engine\ThirdParty
mkdir Engine\Shaders

:: --- Project Directories ---
mkdir Projects
mkdir Projects\Playground
mkdir Projects\Playground\Assets
mkdir Projects\Playground\Source
mkdir Projects\Playground\Config
mkdir Projects\Playground\Cache

:: --- Create Placeholder Files ---
type NUL > Engine\Source\Core\CoreMinimal.h
type NUL > Engine\Source\Runtime\RuntimeModule.h
type NUL > Projects\Playground\Source\GameModule.h

echo.
echo [Success] Folder structure created for Span Engine!
pause