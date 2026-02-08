# 🛠️ Core Architecture

## 1. ECS (Entity Component System)
Span Engineは、メモリ効率とキャッシュヒット率を最大化する **Archetype-based ECS** を採用しています。

### Data Layout
- **Archetype:** 同じコンポーネントの組み合わせを持つエンティティのグループ。
- **Chunk:** 各Archetypeは、16KBごとのメモリオブジェクト「Chunk」に分割して保存されます。
- **SoA (Structure of Arrays):**
  コンポーネントデータはChunk内で配列として連続配置されます。
  これにより、SIMD命令による並列化やプリフェッチが容易になります。

### Entity
- **ID:** 64-bit整数 (32-bit Index + 32-bit Generation)
- `EntityManager` がIDのライフサイクル（生成・破棄・再利用）を管理します。

## 2. Project Structure
```text
Engine/
 Root/
├── Bin/                    # ビルド後の実行ファイル (Editor.exe, Game.exe)
├── Build/                  # CMake中間ファイル (Git無視)
├── Tools/                  # 開発用ツール
│   └── ReflectionParser/   # Python script for code generation
│
├── Engine/                 # [CORE] ゲームエンジン本体
│   ├── Source/
│   │   ├── Core/           # Memory, Math, Container, Log
│   │   ├── Runtime/        # ECS, Physics, Render, Audio, Script systems
│   │   ├── Editor/         # ImGui wrapper, Window implementations
│   │   └── Developer/      # Tests, Profiler
│   ├── ThirdParty/         # JoltPhysics, ImGui, FreeType, DXC, etc.
│   └── Shaders/            # HLSL Source codes
│
└── Projects/               # [USER] ユーザープロジェクト領域
    ├── MyGameProject/      # ゲームごとのルート
    │   ├── Assets/         # Raw assets (png, fbx, wav)
    │   ├── Source/         # User Components & Systems (C++)
    │   ├── Config/         # Engine/Input Settings
    │   └── Cache/          # Imported assets (internal binary format)
    └── ...
```
