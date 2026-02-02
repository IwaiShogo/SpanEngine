# 🎨 Editor & UI/UX Specification

## 1. UI Design Philosophy
- **Framework:** Dear ImGui (Docking Branch)
- **Theme:** "Unity-like Dark Theme" (Rounded corners, Refined padding)
- **Goal:** すべてのコンポーネント操作、デバッグ、アセット管理をGUIで行えるようにする。

## 2. Editor Panels

### `Hierarchy Panel`
シーン内のエンティティをツリー構造で表示する。
- **Source:** `Engine/Source/Editor/Panels/Hierarchy/HierarchyPanel.h`
- **Features:**
  - **Entity Tree:** エンティティの親子関係を可視化。
  - **Drag & Drop:** ドラッグ操作による親子関係の変更（Reparenting）。
  - **Context Menu:** 右クリックでエンティティ作成、削除、コピーなどの操作。

### `Inspector Panel`
選択されたエンティティの詳細情報を編集する。
- **Source:** `Engine/Source/Editor/Panels/Inspector/InspectorPanel.h`
- **Features:**
  - **Component Stack:** エンティティが持つコンポーネントをリスト表示。
  - **Reflection UI:** `SPAN_FIELD` マクロで定義された変数を自動的にUI化。
  - **Add Component:** 新しいコンポーネントの追加ボタン。

### `Scene View Panel`
ゲーム世界の3Dプレビューを表示する。
- **Source:** `Engine/Source/Editor/Panels/SceneView/SceneViewPanel.h`
- **Features:**
  - **Render Target:** DirectX 12の描画結果をImGuiのImageとして表示。
  - **Gizmos:** `ImGuizmo` を使用したオブジェクトの移動・回転・拡大縮小操作。
  - **Aspect Ratio:** ウィンドウサイズ変更時のアスペクト比維持とレターボックス表示。

## 3. Keyboard Shortcuts
| Key | Context | Action |
|---|---|---|
| `Q` | Scene View | 選択モード |
| `W` | Scene View | 移動ギズモ (Translate) |
| `E` | Scene View | 回転ギズモ (Rotate) |
| `R` | Scene View | 拡大縮小ギズモ (Scale) |
| `Ctrl + S` | Global | シーン保存 (TODO) |