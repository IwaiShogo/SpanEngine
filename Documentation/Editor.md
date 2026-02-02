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
ゲーム世界の3Dプレビューと直接操作。

#### **Current Implementation**
- **Render Target:** DirectX 12のバックバッファをテクスチャとしてImGuiウィンドウ内に描画。
- **Gizmos:** `ImGuizmo` を統合し、選択オブジェクトの移動・回転・拡大縮小に対応。
- **Aspect Ratio:** ウィンドウリサイズ時のアスペクト比維持（レターボックス表示）。

#### **Functional Requirements (Planned)**
- [ ] **View Options Toolbar:** パネル上部にオーバーレイツールバーを配置。
  - **Draw Mode:** Shaded / Wireframe / Shaded Wireframe 切り替え。
  - **2D/3D Toggle:** 2D編集モードへの切り替え。
  - **Lighting:** ライティングのOn/Off。
  - **Audio:** オーディオ再生のOn/Off。
- [ ] **Gizmo Settings:**
  - **Pivot/Center:** 操作中心を「オブジェクト原点」か「バウンディングボックス中心」かで切り替え。
  - **Local/Global:** 座標系を「ローカル」か「ワールド」かで切り替え。
  - **Grid Snapping:** 移動・回転のスナップ設定（例: 0.5m単位、15度単位）。
- [ ] **Overlay UI (Scene GUI):**
  - 右上にシーンギズモ（View Cube）を表示し、X/Y/Z軸からの視点へワンクリックで切り替え。
  - グリッド（Grid）と軸（Axis）の描画。

---

## 3.4 Project Browser (Content Browser)
アセットファイルの管理システム。[Planned]

#### **Layout**
- **Left Pane (Folder Tree):** プロジェクトのフォルダ階層（`Assets/` 以下）をツリー表示。
- **Right Pane (Asset Grid):** 選択フォルダ内のファイルをグリッド（アイコン）またはリストで表示。
- **Breadcrumbs:** 現在のパスを表示するパンくずリスト（例: `Assets > Models > Characters`）。

#### **Features**
- **Import Pipeline:** 外部ファイル（.fbx, .png, .wav）がフォルダに追加された際、自動的にインポート処理を行い、エンジン用の中間フォーマットと `.meta` ファイルを生成。
- **Preview:** テクスチャやマテリアルのサムネイルを自動生成して表示。
- **Drag & Drop:**
  - シーンビューへドラッグ → プレハブの配置。
  - Inspectorへドラッグ → アセットの割り当て。
- **Filtering:** 検索バーとタイプフィルター（Show only: Materials, Scripts, Prefabs...）。
- **Asset Creation:** 右クリックで `Create > Material`, `Create > Folder` などのメニュー表示。

---

## 3.5 Console Panel
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

## 3.6 Advanced Creative Tools (Planned) 🚧

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

## 3.7 Debug & Profiling Tools (Planned)

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