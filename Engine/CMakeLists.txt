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

# --- 3. ImGuizmo ---
message(STATUS "Downloading ImGuizmo...")

FetchContent_Declare(
	imguizmo
	GIT_REPOSITORY https://github.com/CedricGuillemet/ImGuizmo.git
	GIT_TAG master
)
FetchContent_MakeAvailable(imguizmo)

set(IMGUIZMO_DIR ${imguizmo_SOURCE_DIR})
file(GLOB IMGUIZMO_SOURCES "${IMGUIZMO_DIR}/*.cpp")

# --- 4. stb (画像読み込み) ---
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

# --- 5. Magic Enum (Enum Reflection) ---
message(STATUS "Downloading Magic Enum...")

FetchContent_Declare(
	magic_enum
	GIT_REPOSITORY https://github.com/Neargye/magic_enum.git
	GIT_TAG v0.9.5
)
FetchContent_MakeAvailable(magic_enum)

# --- 6. nlohmann/json ---
include(FetchContent)
FetchContent_Declare(
	nlohmann_json
	GIT_REPOSITORY https://github.com/nlohmann/json.git
	GIT_TAG v3.11.2
)
FetchContent_MakeAvailable(nlohmann_json)

# ライブラリとして定義
add_library(SpanCore STATIC
	${ENGINE_SOURCES}
	${IMGUI_SOURCES}
	${IMGUIZMO_SOURCES}
 "Source/Runtime/Graphics/Core/RenderTarget.h" "Source/Runtime/Graphics/Core/RenderTarget.cpp" "Source/Runtime/Graphics/Core/GraphicsContext.h" "Source/Runtime/Graphics/Core/GraphicsContext.cpp" "Source/Editor/Panels/EditorPanel.h" "Source/Editor/Panels/SceneView/SceneViewPanel.h" "Source/Editor/Panels/SceneView/SceneViewPanel.cpp" "Source/Runtime/Components/Core/LocalToWorld.h"  "Source/Runtime/Components/Graphics/MeshRenderer.h" "Source/Runtime/Systems/Graphics/EditorCameraSystem.h" "Source/Runtime/Systems/Core/RelationshipSystem.h" "Source/Runtime/Components/Editor/EditorCamera.h" "Source/Editor/SelectionManager.h" "Source/Editor/ImGui/ImGuiUI.h" "Source/Runtime/Reflection/ComponentRegistry.h" "Source/Editor/Panels/Inspector/InspectorPanel.h" "Source/Editor/Panels/Inspector/InspectorPanel.cpp" "Source/Editor/PanelManager.h" "Source/Runtime/Reflection/SpanAttributes.h" "Source/Runtime/Reflection/SpanReflection.h" "Source/Runtime/Components/Core/Tag.h" "Source/Runtime/Components/Core/Layer.h" "Source/Editor/Panels/Hierarchy/HierarchyPanel.h" "Source/Editor/Panels/Hierarchy/HierarchyPanel.cpp" "Source/Runtime/ECS/Kernel/EntityBuilder.h" "Source/Runtime/Components/Core/Active.h" "Source/Core/Containers/FixedString.h" "Source/SpanEditor.h" "Source/Editor/Panels/ProjectBrowser/ProjectBrowserPanel.h" "Source/Editor/Panels/ProjectBrowser/ProjectBrowserPanel.cpp" "Source/Runtime/Resource/AssetMetadata.h" "Source/Runtime/Resource/AssetSerializer.h" "Source/Runtime/Resource/AssetSerializer.cpp" "Source/Editor/Utils/EditorFileSystem.h" "Source/Editor/Utils/EditorFileSystem.cpp" "Source/Editor/ImGui/ImGuiUI.cpp" "Source/Editor/Core/ICommand.h" "Source/Editor/Commands/FileCommands.h" "Source/Runtime/Resource/AssetRegistry.h" "Source/Runtime/Resource/AssetRegistry.cpp" "Source/Editor/Utils/DirectoryWatcher.h" "Source/Editor/Utils/DirectoryWatcher.cpp" "Source/Runtime/Core/TagManager.h" "Source/Runtime/Core/LayerManager.cpp" "Source/Runtime/Scene/SceneSerializer.cpp" "Source/Editor/Utils/FileDialog.h" "Source/Editor/Utils/FileDialog.cpp" "Source/Runtime/Components/Core/IDComponent.h" "Source/Runtime/Scene/Scene.h" )

# インクルードパスの設定
target_include_directories(SpanCore PUBLIC
	Source
	Source/Core
	Source/Runtime
	Source/Editor
	${stb_SOURCE_DIR}
	${IMGUI_DIR}
	${IMGUI_DIR}/backends
	${IMGUIZMO_DIR}
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
	magic_enum::magic_enum
	nlohmann_json::nlohmann_json
)

# コンパイルオプション
if(MSVC)
	target_compile_options(SpanCore PRIVATE /W4 /MP /permissive-)
	target_compile_definitions(SpanCore PRIVATE _UNICODE UNICODE NOMINMAX)
endif()

# フィルタ設定 (Visual Studio上のフォルダ分け)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/Source PREFIX "Source" FILES ${ENGINE_SOURCES})
source_group("ImGui" FILES ${IMGUI_SOURCES})
source_group("ImGuizmo" FILES ${IMGUIZMO_SOURCES})
