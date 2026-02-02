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
 ┣━━ Source/
 ┃    ┣━━ Core/           # Memory, Math, Input, Log
 ┃    ┣━━ Runtime/        # ECS, Physics, Render, Components
 ┃    ┣━━ Editor/         # ImGui Panels, Tools
 ┃    ┗━━ Developer/      # Tests, Profiler
 ┗━━ ThirdParty/          # JoltPhysics, ImGui, DXC, etc...