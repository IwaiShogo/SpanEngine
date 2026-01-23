# ==============================================================================
# Engine/CMakeLists.txt
# ==============================================================================

# --- 1. Assimp をインターネットから自動取得 (FetchContent) ---
include(FetchContent)

message(STATUS "Downloading Assimp... (This may take a while)")

FetchContent_Declare(
    assimp
    GIT_REPOSITORY https://github.com/assimp/assimp.git
    GIT_TAG v5.4.3  # 使いたいバージョンを指定
)

# Assimpのビルド時間を短縮するための設定 (テストやツールは作らない)
set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "" FORCE)
set(ASSIMP_NO_EXPORT ON CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ZLIB ON CACHE BOOL "" FORCE) # 内蔵Zlibを使う

# ダウンロードとビルドの実行
FetchContent_MakeAvailable(assimp)

# ------------------------------------------------------------------------------

# --- 2. stb (画像読み込み) ---
FetchContent_Declare(
    stb
    GIT_REPOSITORY https://github.com/nothings/stb.git
    GIT_TAG master
)
FetchContent_MakeAvailable(stb)

# ソースファイルの収集
file(GLOB_RECURSE ENGINE_SOURCES 
    "Source/*.cpp" 
    "Source/*.h"
)

# ライブラリとして定義
add_library(SpanCore STATIC ${ENGINE_SOURCES} "Source/Runtime/Graphics/ModelLoader.cpp" "Source/Runtime/Graphics/Resources/Texture.cpp")

# インクルードパスの設定
target_include_directories(SpanCore PUBLIC 
    Source
    Source/Core
    Source/Runtime
    Source/Editor
    ${stb_SOURCE_DIR}
)

# プリコンパイル済みヘッダー
target_precompile_headers(SpanCore PUBLIC Source/Core/CoreMinimal.h)

# ------------------------------------------------------------------------------
# リンク設定
# ------------------------------------------------------------------------------
target_link_libraries(SpanCore PUBLIC 
    d3d12.lib 
    dxgi.lib 
    d3dcompiler.lib
    assimp::assimp  # vcpkgなしでリンクできます
)

# コンパイルオプション
if(MSVC)
    target_compile_options(SpanCore PRIVATE /W4 /MP /permissive-)
    target_compile_definitions(SpanCore PRIVATE _UNICODE UNICODE NOMINMAX)
endif()

# フィルタ設定
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/Source PREFIX "Source" FILES ${ENGINE_SOURCES})