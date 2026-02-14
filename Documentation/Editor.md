# 🎨 Editor & UI/UX Specification

## 1. Design Philosophy
- **Framework:** Dear ImGui (Docking Branch)
- **Theme:** "Unity-like Dark Theme"
  - **Palette:** Dark Gray backgrounds (`#202020`), Lighter panels (`#383838`), Blue accents (`#3584e4`).
  - **Typography:** Roboto / Noto Sans (Clean, Anti-aliased).
  - **Icons:** FontAwesome integration for visual cues.
- **Goal:** すべてのコンポーネント操作、デバッグ、アセット管理をGUIで行えるようにし、プログラマ以外のクリエイターも扱えるUXを提供する。

---

## 2. Keyboard Shortcuts (Key Bindings)

エディタ操作を高速化するためのショートカットキー定義。

### Global
| Key Combination | Context | Action |
| :--- | :--- | :--- |
| `Ctrl + S` | Global | シーンを保存 (Save Scene) |
| `Ctrl + Shift + S` | Global | 別名でシーンを保存 (Save Scene As...) |
| `Ctrl + N` | Global | 新規シーン作成 (New Scene) |
| `Ctrl + Z` | Global | 元に戻す (Undo) [Planned] |
| `Ctrl + Y` | Global | やり直し (Redo) [Planned] |
| `Delete` | Global | 選択中のエンティティ/アセットを削除 |
| `F2` | Global | 選択中の項目の名前変更 (Rename) |

### Scene View & Tools
| Key | Context | Action |
| :--- | :--- | :--- |
| `Q` | Viewport | **View Tool** (ハンドツール/選択のみ) |
| `W` | Viewport | **Move Tool** (移動ギズモ) |
| `E` | Viewport | **Rotate Tool** (回転ギズモ) |
| `R` | Viewport | **Scale Tool** (拡大縮小ギズモ) |
| `T` | Viewport | **Rect Tool** (2D/UI用矩形編集) [Planned] |
| `F` | Viewport | **Focus** (選択したオブジェクトを画面中央に捉える) |
| `Right Click + WASD` | Viewport | **Fly Camera** (一人称視点での移動) |
| `Alt + Left Drag` | Viewport | **Orbit** (注視点周りの回転) |
| `Alt + Right Drag` | Viewport | **Zoom** (ズームイン/アウト) |
| `Alt + Middle Drag` | Viewport | **Pan** (平行移動) |

---

## 3. Core Panels Specification

### 3.1 Hierarchy Panel
シーン内のエンティティ構造を管理するメインパネル。

#### **Current Implementation (Implemented)**
- **Entity Tree:** 親子関係（Relationship）に基づいたツリー表示。
- **Drag & Drop:**
  - エンティティをドラッグして他のエンティティの子にする (Reparenting)。
  - 何もない場所にドロップしてルートエンティティにする (Unparenting)。
- **Context Menu:** 右クリックで「Create Empty」「Delete」「Copy/Paste」等の操作。

#### **Functional Requirements (Planned)**
- [ ] **Advanced Search Bar:**
  - パネル上部に検索バーを配置。
  - **Fuzzy Search:** 部分一致、大文字小文字無視での検索。
  - **Filter Support:** `t:Player` (Tag検索), `c:MeshRenderer` (Component検索) などのフィルタ機能。
- [ ] **Create Menu Expansion:**
  - 右クリックメニューにプリミティブ作成を追加 (`Create 3D Object` -> `Cube`, `Sphere`, `Capsule`, `Plane` etc.)。
- [ ] **Static / Visibility Toggles:**
  - 各行の右側に「目のアイコン（表示切替）」と「指のアイコン（選択不可）」を配置。

#### **Visual UX Improvements (Planned)**
- [ ] **Iconography:**
  - エンティティの種類に応じたアイコンを表示（例: 🎥Camera, 💡Light, 📦Mesh）。
  - ユーザー作成スクリプト（Component）がついている場合、📜スクリプトアイコンを表示。
- [ ] **Tree Guides:**
  - 親子関係の階層を視覚的に追えるよう、縦の罫線（Indent Guides）を描画する。
- [ ] **Color Coding:**
  - **Prefab Instance:** テキストを青色 (`#50c4f2`) で表示。
  - **Missing Prefab:** テキストを赤色で表示。
- [ ] **Metadata Display:**
  - 設定により、エンティティ名の横に「Tag」や「Layer」を薄い文字色で併記可能にする。
  - 子オブジェクトの数（Child Count）をフォルダアイコン横に表示 `(3)`。
- [ ] **Status Indicators:**
  - **IsStatic:** Staticフラグが立っているエンティティに `S` アイコンを表示。
  - **Lock:** 編集ロックされているエンティティの背景色を変更。

---

### 3.2 Inspector Panel
選択されたエンティティまたはアセットの詳細プロパティ編集。

#### **Header Area (Standard)**
すべてのエンティティ共通のヘッダー情報。
- **Active Checkbox:** 左上のチェックボックスでエンティティの有効/無効を切り替え（階層全体に影響）。
- **Entity Name:** 名前編集フィールド。
- **Static Toggle:** "Static" チェックボックス（BatchingやBaking用）。
- **Tag / Layer:**
  - **Tag:** ドロップダウンから選択。`Add Tag...` で新規登録ダイアログへ。
  - **Layer:** ドロップダウン（ビットマスク対応が望ましい）で所属レイヤーを選択。

#### **Component Stack**
- **Component Container:** 各コンポーネントを折りたたみ可能なヘッダー付きカードとして描画。
- **Reflection UI:** `SpanReflection` システムにより、型に応じたUIを自動生成。
  - `int`, `float`: ドラッグ可能な数値入力（Dragger）。
  - `bool`: チェックボックス。
  - `Vector3`: X/Y/Z のカラーラベル付き入力フィールド。
  - `Color`: カラーピッカー。
  - `Asset Ptr`: ドラッグ＆ドロップ受け入れボックス + 参照ボタン。
- **Component Options:** 各コンポーネント右上の歯車アイコン ⚙️。
  - `Reset`: 値を初期値に戻す。
  - `Remove Component`: コンポーネントを削除。
  - `Copy/Paste Component Values`: 値のコピー。

#### **Footer Area**
- **Add Component Button:**
  - クリックで検索窓付きのドロップダウンを表示。
  - カテゴリ分け（Core, Graphics, Physics, Audio, Custom...）されたコンポーネント一覧から追加。

#### **Planned Features**
- [ ] **Multi-Select Editing:** 複数エンティティ選択時、共通するコンポーネントの値を一括編集する機能。値が異なる場合は `--` と表示。
- [ ] **Debug Mode:** 通常隠されているPrivateフィールドや生のIDを表示するモード切替。
- [ ] **Lock Inspector:** 別のエンティティを選択してもInspectorの内容を固定する鍵アイコン 🔒。

---

### 3.3 Scene View Panel
ゲーム世界の3Dプレビューとオブジェクトの直接操作を行うメインエディタ。

#### **Current Implementation & Specifications**

**1. Rendering Features**
- **Infinite Grid:** カスタムシェーダー (`EditorGrid.hlsl`) による無限グリッド描画。
  - カメラ位置に追従し、無限遠までグリッドを表示。
  - 距離に応じたフェードアウト処理 (Distance Fading)。
  - 1m単位の細線と10m単位の太線を視認性良くブレンド。
  - 深度テストを有効化し、オブジェクトの前後関係を正しく描画。
- **World Origin Axes:** 原点 (0,0,0) を示す太いX軸（赤）とZ軸（青）の表示。

**2. Camera Navigation (Editor Camera)**
- **Fly Mode:** `Right-Click` ホールド + `W, A, S, D` で前後左右移動、`Q, E` で垂直昇降。
- **Look Around:** `Right-Click` ホールド + マウス移動で視点回転（Pitch/Yaw）。
- **Acceleration Control:**
  - `Shift` キー押下で移動速度を徐々に加速（Smooth Lerp）。
  - キー離脱時は慣性を残さず即座に停止（Instant Stop）。
- **Zoom & Speed:**
  - **Scroll:** 前後移動（Perspective時）または ズームサイズ変更（Orthographic時）。
  - **Right-Click + Scroll:** カメラの基本移動速度を変更 (0.1x ~ 200.0x)。
- **Speed Overlay:** 速度変更時、画面中央に現在の倍率（例: `Speed: 5.0x`）を一時的にオーバーレイ表示。

**3. Integrated Toolbar (Overlay UI)**
パネル上部に統合された操作バー。ウィンドウ領域を消費せず、オーバーレイとして描画。
- **Tools:** トランスフォーム操作切替 (`Q: View`, `W: Translate`, `E: Rotate`, `R: Scale`)。
- **Coordinate Space:** 座標系の切り替え (`Local` / `Global`)。
- **Snapping:** グリッドスナップの有効化 (`S`)。
- **Aspect Ratio:** 解像度比率の選択 (`Free`, `16:9`, `16:10`, `4:3`, `21:9`)。

**4. Scene Gizmo (Unity-Style)**
画面右上に配置された3D軸コントローラー。
- **Axis Navigation:** X/Y/Z軸をクリックすることで、その方向からの視点へカメラをスナップ。現在の注目点（ピボット）を中心に回転。
- **Projection Toggle:** 中心をクリックして 透視投影 (`Perspective`) と 平行投影 (`Orthographic`) を切り替え。
- **Dynamic Resizing:** パネルサイズに応じて適切な大きさに自動リサイズ。

#### **Functional Requirements (Planned)**
- [ ] **Visual Feedback:**
  - **Selection Outline:** 選択中のオブジェクトの輪郭をオレンジ色などで発光表示（ステンシルまたはポストプロセス）。
  - **Gizmo Icons:** ライト、カメラ、オーディオソースなど、メッシュを持たないオブジェクトのアイコン表示（ビルボード）。
- [ ] **Advanced Navigation:**
  - **Frame Selected (F):** 選択中のオブジェクトを画面中央に捉えるフォーカス機能。
  - **Box Selection:** マウスドラッグによる矩形選択。
- [ ] **View Options:**
  - **Shading Mode:** Lit / Unlit / Wireframe の描画モード切り替え。
  - **Stats Overlay:** FPS、ポリゴン数、ドローコール数の表示。
  - 
---

### 3.4 Game View Panel
実際のゲームプレイ体験をシミュレーションし、最終的なレンダリング結果を確認するビュー。

#### **Current Implementation**
- **Render Target:** メインカメラ（`MainCamera`タグを持つエンティティ）の視点をレンダリング。
- **Input Handling:** アクティブ時のみキーボード・マウス入力をゲームロジックへ転送。

#### **Functional Requirements (Planned)**

**1. Playback Controls (Toolbar)**
パネル上部に統合された再生コントロールバー。
- [ ] **Play/Stop:** ゲームループの開始と終了。
- [ ] **Pause/Resume:** ゲームロジックの一時停止と再開（レンダリングは継続）。
- [ ] **Frame Step:** 一時停止中に1フレームだけ進行させるコマ送り機能（デバッグ用）。
- [ ] **Error Pause:** エラーログ発生時に自動的に一時停止するトグルスイッチ。

**2. Display & Resolution Settings**
多様なデバイス環境をエミュレートするための表示設定。
- [ ] **Aspect Ratio / Resolution Presets:**
  - `Free Aspect` (ウィンドウサイズ依存)
  - `1920x1080 (FHD)`, `2560x1440 (QHD)`, `3840x2160 (4K)`
  - `Portrait (9:16)` (モバイル用)
  - `Custom` (ユーザー定義)
- [ ] **Scale Slider:** 解像度を固定したまま、表示倍率を変更するスライダー (0.1x ~ 5.0x)。
- [ ] **Low Resolution Aspect:** 低解像度レンダリング時のピクセルパーフェクト表示とバイリニア補間の切り替え。
- [ ] **VSync Toggle:** 垂直同期の強制 On/Off。

**3. Debugging & Visualization (G-Buffer)**
レンダリングパイプラインの中間バッファを可視化し、グラフィックスデバッグを行う機能。
- [ ] **Render Mode Dropdown:**
  - **Final Color:** 最終的なレンダリング結果。
  - **Albedo:** アルベド（ベースカラー）のみ表示。
  - **Normal:** ワールド法線/ビュー法線の可視化。
  - **Depth:** 深度バッファのグレースケール表示 (Linear/Non-Linear)。
  - **Roughness / Metallic:** PBRマテリアルプロパティの可視化。
  - **Overdraw:** 描画負荷（オーバードロー）をヒートマップで表示。
  - **Wireframe:** ポリゴン分割の可視化オーバーレイ。
- [ ] **Gizmos Toggle:** ゲームビュー内でのデバッグ描画（コライダー枠、レイキャスト線など）の On/Off。

**4. Stats Overlay (Performance HUD)**
リアルタイムのパフォーマンス情報を半透明オーバーレイで表示。
- [ ] **Basic Stats:** FPS (Min/Max/Avg), Frame Time (ms)。
- [ ] **Rendering Stats:** Draw Calls, Triangles, Vertices count。
- [ ] **Memory:** System RAM, GPU VRAM usage。
- [ ] **Graph Visualizer:** フレームタイムの推移を簡易グラフでリアルタイム表示。

**5. Device & Network Emulation**
実機環境やネットワーク環境をシミュレートする機能。
- [ ] **Network Simulation:** ラグ（Latency）、パケットロス、ジッターを擬似的に発生させるスライダー。
- [ ] **Input Emulation:** マウス入力をタッチ入力としてエミュレートするトグル。
- [ ] **Safe Area Guide:** TVやノッチ付きスマホのためのセーフエリアガイド線の表示。

**6. Capture Tools**
- [ ] **Screenshot:** 現在のフレームを高解像度スクリーンショットとして保存（アルファチャンネル対応）。
- [ ] **Recorder:** 直近30秒のゲームプレイをGIFまたはMP4として保存。

---

## 3.5 Project Browser (Content Browser)
アセットファイルの管理システム。Windowsのエクスプローラーライクな操作感と、ゲームエンジン特有のインポート処理を統合します。

### **Layout**
- **Two-Pane Interface:**
  - [x] **Left Pane (Folder Tree):** プロジェクトのフォルダ階層（`Assets/` 以下）をツリー表示。
  - [x] **Right Pane (Asset Grid/List):** 現在選択されているフォルダ内のファイル一覧を表示。
- **Navigation Bar:**
  - [x] **Breadcrumbs:** 現在のパスを表示するパンくずリスト。
  - [ ] **History Controls:** `<` (戻る) `>` (進む) ボタン。 (Backspaceキーのみ実装済)
  - [ ] **Search Field:** 名前検索およびフィルタリング用入力欄。 (簡易実装済、高機能化が必要)
- **Bottom Bar:**
  - [ ] **Item Count:** 現在表示中のアイテム数を表示。
  - [x] **Zoom Slider:** アイコンの表示サイズを動的に変更するスライダー。

### **Features**

#### **1. Selection & Manipulation (Interaction)**
- [x] **Multi-Select:** `Ctrl + Click`（個別追加）、`Shift + Click`（範囲選択）。
- [ ] **Rubber Band Selection:** マウスドラッグによる矩形範囲選択。
- [ ] **Keyboard Navigation:** 矢印キーでのフォーカス移動。
- [x] **Renaming:** `F2` キーまたはコンテキストメニュー。 (.meta更新対応)
- [x] **Delete:** `Delete` キーで削除確認ダイアログを表示。 (.meta削除対応)

#### **2. Drag & Drop Workflow**
- [x] **External to Engine (Import):**
  - エクスプローラーからのインポート。.metaの自動生成。
- [ ] **Internal Move:**
  - **[重要]** Browser内でファイルをドラッグし、左側のフォルダツリーやフォルダアイコンへドロップして移動させる。
  - `.meta` ファイルも同時に移動させる必要がある。
- [x] **Asset to Scene:**
  - モデル(.fbx) → シーンビューへの配置。
- [ ] **Asset to Inspector:**
  - アセット → コンポーネントプロパティへの割り当て。

#### **3. Asset Creation (Context Menu)**
- [x] **Folder:** 新規フォルダ。
- [x] **C++ Component/System:** テンプレート生成。
- [ ] **Shader:** HLSLシェーダーファイルの作成。
- [ ] **Material:** デフォルトまたは選択したシェーダーベースのマテリアルアセット (.mat)。
- [ ] **Scene:** 空の新規シーン。
- [x] **Show in Explorer:** OSのファイラーで該当フォルダを開く。

#### **4. Visualization & Filtering**
- [x] **Thumbnail Generation (Texture):** 画像のサムネイル表示。
- [ ] **Thumbnail Generation (Material/Model):** 3Dプレビュー画像のキャッシュ生成。
- [x] **View Modes (Grid):** アイコン主体のグリッド表示。
- [ ] **View Modes (List):** 詳細リスト表示。
- [ ] **Advanced Search:** タイプフィルタ (`t:Texture`)、ラベルフィルタ。

### **🚀 Additional Features (Proposed for Commercial Grade)**
商用レベルに引き上げるために追加すべき機能です。

- **Asset Database / Registry:**
  - 全アセットの GUID と ファイルパス の対応表をメモリ上に保持するシステム。
  - ファイル移動やリネームが発生しても、GUID経由で参照を維持するために必須。
- **Directory Watcher (Hot Reload):**
  - エンジン外（VSCodeやPhotoshopなど）でファイルが変更されたことを検知し、自動的にリロード・インポートする機能。
- **Favorites / Collections:**
  - 実際のフォルダ階層とは別に、よく使うアセットをブックマークする機能。

---

## 3.6 Console Panel
ログとエラー情報の集約。[Planned]

#### **Features**
- **Log Types:**
  - ⚪ **Info:** 一般的な情報。
  - 🟡 **Warning:** 注意（アセット欠損など）。
  - 🔴 **Error:** クリティカルなエラー、例外。
- **Filtering:** 種類ごとの表示ON/OFF切り替えボタン。
- **Collapse:** 同じメッセージが連続した場合、1行にまとめて件数を表示。
- **Stack Trace:** エラークリック時に下部にスタックトレースを表示。
- **Source Jump:** ダブルクリックでVisual Studioの該当行を開く。

---

## 3.7 Advanced Creative Tools (Planned) 🚧

UnityのUXを再現するための、高度な専用エディタウィンドウ群。

### **Shader Graph Editor**
ノードベースのビジュアルシェーダーエディタ。
- **Canvas:** `ImNodes` または `ImFlow` を使用した無限キャンバス。
- **Master Node:** PBR (Physical), Unlit, VFX 用の最終出力ノード。
- **Blackboard:** マテリアルInspectorに公開するプロパティ（Color, Texture, Float）の定義パネル。
- **Preview:** 編集中のシェーダーをリアルタイムで適用した3Dモデル（球体/立方体）のプレビューウィンドウ。
- **Code Generation:** グラフからHLSLコードを自動生成し、コンパイルエラーがあれば該当ノードを赤くハイライト。

### **Animator Controller**
キャラクターのアニメーションステート管理エディタ。
- **State Machine Graph:**
  - **States:** アニメーションクリップ（Idle, Run, Jump）を表すノード。
  - **Transitions:** ステート間を繋ぐ矢印。クリックでブレンド時間や条件（Conditions）を設定。
- **Parameters Panel:** `IsRunning (bool)`, `Speed (float)` などの変数リスト。
- **Layers:** 上半身・下半身などを分離するためのレイヤー管理タブ。Mask設定対応。
- **Blend Tree Editor:** 移動速度に応じたモーションブレンド（1D/2D）を編集するサブグラフ。

### **Audio Mixer**
サウンドのバス（Bus）管理とエフェクト処理。
- **Groups:** Master, BGM, SE, Voice などの階層構造を持つグループ管理。
- **Channel Strips:**
  - 各グループのボリュームフェーダー、dBメーター。
  - Mute / Solo / Bypass ボタン。
- **Effect Rack:** 各グループにインサート可能なエフェクト一覧（EQ, Compressor, Reverb）。
- **Send/Return:** リバーブなどの共有エフェクト用ルーティング設定。

### **UI Editor**
2Dインターフェース（HUD）作成専用モード。
- **Canvas View:** 3Dシーンとは独立した、スクリーン空間の編集ビュー。
- **Rect Transform Tool:** UI専用のアンカー（Anchor）、ピボット（Pivot）編集ギズモ。
  - プリセット（左上揃え、ストレッチ等）の視覚的選択メニュー。
- **Hierarchy Integration:** Hierarchyパネル内でCanvas以下の要素は順序が描画順（Sort Order）になるよう特別扱い。

### **NavMesh Baker**
AIナビゲーション用データの生成設定。
- **Bake Settings:**
  - **Agent Radius:** 通行可能な最小半径。
  - **Agent Height:** 通行可能な最小の高さ。
  - **Max Slope:** 登れる坂の角度。
  - **Step Height:** 乗り越えられる段差の高さ。
- **Object Filtering:** ベイク対象を `All`, `MeshRenderers`, `Physics Colliders` から選択。
- **Visualization:** シーンビュー上にベイク結果（歩行可能エリア）を青いオーバーレイで表示。

---

## 3.8 Debug & Profiling Tools (Planned)

### **System Monitor**
ECSパフォーマンスの可視化。
- **System List:** 登録されている全Systemを一覧表示。
- **Metrics:**
  - **Execution Time:** 各Systemの更新にかかった時間（ms）。
  - **Entities Processed:** 処理対象となったEntity数。
- **Control:** 特定のSystemを一時的にDisableにするチェックボックス。

### **Archetype Viewer**
メモリレイアウトのデバッグツール。
- **Visualization:** 現在メモリ上に存在するArchetype（コンポーネントの組み合わせ）を一覧表示。
- **Chunk Info:** 各Archetypeが確保しているChunk数と、メモリ使用率（Occupancy）を表示。フラグメンテーションの確認に使用。

### **Profiler**
詳細なパフォーマンス分析。
- **Timeline:** CPU, GPU, Memoryの使用量を時系列グラフで表示。
- **Frame Debugger:** 1フレーム内のドローコールを順次再生し、レンダリングパイプラインの状態を確認。
- **Markers:** `SPAN_PROFILE_SCOPE("Name")` で埋め込んだ区間の処理時間を階層表示（Flame Graph）。
