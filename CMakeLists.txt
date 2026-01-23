cmake_minimum_required(VERSION 3.20)

# プロジェクト名とバージョン
project(SpanEngine VERSION 0.1.0 LANGUAGES CXX)

# C++バージョンの指定 (C++20必須)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 出力ディレクトリの設定
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/Bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/Bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/Build/Lib)

# フォルダ構成によるグループ化 (Visual Studioのソリューション用)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# --- オプション設定 ---
option(BUILD_EDITOR "Build the Span Engine Editor" ON)
option(BUILD_TESTS "Build Unit Tests" OFF)

# --- エンジン本体の追加 ---
# Engineフォルダ内のCMakeLists.txtを読みに行く設定
add_subdirectory(Engine)

# --- ユーザープロジェクトの追加 (今回はPlayground) ---
# 将来的にはランチャーで動的に切り替えるが、今は直接指定
add_subdirectory(Projects/Playground)

message(STATUS "Span Engine Configuration Done.")