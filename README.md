# Span Engine

<div align = "center">

**"Editor First, Data Oriented, Native Performance."**

Unityの優れたUXと、DOTS (Data-Oriented Technology Stack) を凌駕するパフォーマンスを融合させた、<br>
次世代 C++ / DirectX 12 ゲームエンジン。

[機能一覧]・[ドキュメント]・[ロードマップ]

</div>

---

## 📖 目次 (Documentation)

プロジェクトの詳細な仕様については、以下のドキュメントを参照してください。

| ドキュメント | 内容 |
| :--- | :--- |
| **[🛠️ Architecture](./Documentation/Architecture.md)** | エンジンのコア設計、ECSアーキテクチャ、メモリ管理戦略 |
| **[🧱 Components](./Documentation/Components.md)** | 全コンポーネントのデータ構造とInspector仕様 |
| **[⚙️ Systems](./Documentation/Systems.md)** | 各システムのロジック、実行順序、依存関係 |
| **[🎨 Editor & UX](./Documentation/Editor.md)** | エディタの機能、パネル仕様、操作方法 |
| **[📅 Roadmap](./Documentation/Roadmap.md)** | 開発進捗、既知のバグ、将来の機能予定 |

## 🚀 プロジェクト概要 (Overview)
- **Engine Name:** Span Engine
- **Target Platform:** Windows (DirectX 12)
- **Language:** C++20 (Core & User Logic)
- **Philosophy:**
    - **Data-Oriented:** Cache Missを撲滅し、極限のパフォーマンスを追求。
    - **Editor First:** 全ての機能はGUIで操作可能であること。
    - **No GC Spikes:** メモリを完全制御し、ガベージコレクションによるスパイクを排除。

## 🔧 ビルド方法 (Build)
1. リポジトリをCloneする
2. `SetupProject.bat` を実行 (依存ライブラリの展開)
3. `GenerateProject.bat` を実行 (CMakeによるソリューション生成)
4. `SpanEngine.sln` を開き、ビルドして実行
