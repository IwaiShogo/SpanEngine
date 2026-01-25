# ==============================================================================
# Engine/CMakeLists.txt
# ==============================================================================

# --- 1. Assimp をインターネットから自動取得 (FetchContent) ---
include(FetchContent)

message(STATUS "Downloading Assimp... (This may take a while)")

FetchContent_Declare(
	assimp
	GIT_REPOSITORY https://github.com/assimp/assimp.git
	GIT_TAG v5.4.3
)

set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "" FORCE)
set(ASSIMP_NO_EXPORT ON CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ZLIB ON CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(assimp)

# --- 2. ImGui (Docking Branch) を自動取得 ---
message(STATUS "Downloading ImGui...")

FetchContent_Declare(
	imgui
	GIT_REPOSITORY https://github.com/ocornut/imgui.git
	GIT_TAG docking
)
FetchContent_MakeAvailable(imgui)

# ソースファイルのリストアップ
set(IMGUI_DIR ${imgui_SOURCE_DIR})
file(GLOB IMGUI_SOURCES
	"${IMGUI_DIR}/*.cpp"
	"${IMGUI_DIR}/backends/imgui_impl_dx12.cpp"
	"${IMGUI_DIR}/backends/imgui_impl_win32.cpp"
)

# --- 3. stb (画像読み込み) ---
FetchContent_Declare(
	stb
	GIT_REPOSITORY https://github.com/nothings/stb.git
	GIT_TAG master
)
FetchContent_MakeAvailable(stb)

# ソースファイルの収集 (これでSourceフォルダ以下の全cpp/hが含まれます)
file(GLOB_RECURSE ENGINE_SOURCES
	"Source/*.cpp"
	"Source/*.h"
)

# ライブラリとして定義
add_library(SpanCore STATIC 
	${ENGINE_SOURCES} 
	${IMGUI_SOURCES}
 "Source/Runtime/Graphics/Core/RenderTarget.h" "Source/Runtime/Graphics/Core/RenderTarget.cpp" "Source/Runtime/Graphics/Core/GraphicsContext.h" "Source/Runtime/Graphics/Core/GraphicsContext.cpp" "Source/Editor/Panels/EditorPanel.h" "Source/Editor/Panels/SceneViewPanel.h" "Source/Editor/Panels/SceneViewPanel.cpp" "Source/Runtime/Components/Core/LocalToWorld.h" "Source/Runtime/Systems/Core/TransformSystem.h" "Source/Runtime/Components/Graphics/MeshRenderer.h")

# インクルードパスの設定
target_include_directories(SpanCore PUBLIC 
	Source
	Source/Core
	Source/Runtime
	Source/Editor
	${stb_SOURCE_DIR}
	${IMGUI_DIR}
	${IMGUI_DIR}/backends
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
	assimp::assimp
	xinput
)

# コンパイルオプション
if(MSVC)
	target_compile_options(SpanCore PRIVATE /W4 /MP /permissive-)
	target_compile_definitions(SpanCore PRIVATE _UNICODE UNICODE NOMINMAX)
endif()

# フィルタ設定 (Visual Studio上のフォルダ分け)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/Source PREFIX "Source" FILES ${ENGINE_SOURCES})
source_group("ImGui" FILES ${IMGUI_SOURCES})