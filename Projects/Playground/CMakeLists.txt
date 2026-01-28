# Projects/Playground/CMakeLists.txt

file(GLOB_RECURSE GAME_SOURCES "Source/*.cpp" "Source/*.h")

# 実行ファイル (EXE) を生成
add_executable(Playground ${GAME_SOURCES})

# エンジン本体とリンク
target_link_libraries(Playground PRIVATE SpanCore)

# SpanCore内の自動登録機能を有効にするための強制リンク
if(MSVC)
    target_link_options(Playground PRIVATE "/WHOLEARCHIVE:$<TARGET_FILE:SpanCore>")
endif()

# インクルードパス
target_include_directories(Playground PRIVATE Source)

# VS上の表示をフォルダ構造と一致させる
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} PREFIX "" FILES ${GAME_SOURCES})

# ------------------------------------------------------------------------------
# シェーダーフォルダを実行ファイルの出力ディレクトリに自動コピーする
# ------------------------------------------------------------------------------
add_custom_command(TARGET Playground POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${CMAKE_SOURCE_DIR}/Engine/Shaders"
    "$<TARGET_FILE_DIR:Playground>/Shaders"
    COMMENT "Copying Shaders to Output Directory..."
)

# ------------------------------------------------------------------------------
# Assetsフォルダを実行ファイルの出力ディレクトリに自動コピーする
# ------------------------------------------------------------------------------
add_custom_command(TARGET Playground POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${CMAKE_CURRENT_SOURCE_DIR}/Assets"
    "$<TARGET_FILE_DIR:Playground>/Assets"
    COMMENT "Copying Assets to Output Directory..."
)