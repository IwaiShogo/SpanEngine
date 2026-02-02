# 🧱 Standard Components Specification

Span Engineの標準コンポーネントライブラリ。
すべてのコンポーネントは **POD (Plain Old Data)** であり、ロジックを持ちません。

---

## 1. Core Components (Implemented)

### `Tag`
エンティティを分類・検索するための識別タグ。
- **Source:** `Engine/Source/Runtime/Components/Core/Tag.h`
- **Fields:** `string Value` ("Untagged")

### `Layer`
物理衝突やレンダリングのフィルタリングに使用するビットマスク。
- **Source:** `Engine/Source/Runtime/Components/Core/Layer.h`
- **Fields:** `int Value` (0-31)
- **Defined Layers:** `Default(0)`, `TransparentFX(1)`, `IgnoreRaycast(2)`, `Water(3)`, `UI(4)`

### `Active`
エンティティの有効/無効状態を制御するフラグ。
- **Source:** `Engine/Source/Runtime/Components/Core/Active.h`
- **Fields:** `bool Value` (true/false)

### `Transform`
ワールド空間における位置、回転、スケール。
- **Source:** `Engine/Source/Runtime/Components/Core/Transform.h`
- **Fields:** `Vector3 Position`, `Quaternion Rotation`, `Vector3 Scale`
- **Editor:** RotationはInspector上でEuler角(度数法)として表示。

### `Relationship`
階層構造（親子関係）を構築するためのリンクデータ。
- **Source:** `Engine/Source/Runtime/Components/Core/Relationship.h`
- **Fields:** `Entity Parent`, `FirstChild`, `PrevSibling`, `NextSibling`

### `LocalToWorld`
計算済みのワールド変換行列キャッシュ。
- **Source:** `Engine/Source/Runtime/Components/Core/LocalToWorld.h`
- **Fields:** `Matrix4x4 Value`

---

## 2. Graphics Components (Implemented)

### `Camera`
レンダリングの視点と投影設定。
- **Source:** `Engine/Source/Runtime/Components/Graphics/Camera.h`
- **Fields:** `float Fov`, `float NearClip`, `float FarClip`

### `MeshFilter`
描画するメッシュリソースの参照。
- **Source:** `Engine/Source/Runtime/Components/Graphics/MeshFilter.h`
- **Fields:** `Mesh* mesh`

### `MeshRenderer`
メッシュの描画設定（マテリアル、影）。
- **Source:** `Engine/Source/Runtime/Components/Graphics/MeshRenderer.h`
- **Fields:** `Material* material`, `bool castShadows`, `bool receiveShadows`

---

## 3. Graphics Components (Planned) 🚧

### `Light`
シーン内の光源。
- **Fields:**
  - `Type`: Directional, Point, Spot, Area
  - `Color`: 光の色
  - `Intensity`: 光の強さ
  - `Range`: 影響範囲 (Point/Spot)

### `MaterialOverride`
インスタンスごとのマテリアルパラメータ上書き用。
- **Fields:** `Map<string, float/Vector4> Parameters`

### `LightProbeGroup`
間接照明（GI）用のSH（球面調和関数）データ。
- **Fields:** `List<SH9> Coefficients`, `List<Vector3> Positions`

### `ReflectionProbe`
環境マップ用のCubemapデータ。
- **Fields:** `Cubemap Texture`, `Box Bounds`

### `PostProcessVolume`
画面効果の設定エリア。
- **Fields:** `bool IsGlobal`, `float Weight`, `BloomSettings`, `ToneMappingSettings`

### `Decal`
投影テクスチャ設定。
- **Fields:** `Texture* Albedo`, `Texture* Normal`

### `ParticleEmitter`
GPUパーティクル設定。
- **Fields:** `int MaxParticles`, `float EmissionRate`, `Curve SizeOverLifetime`

### `LODGroup`
カメラ距離に応じたメッシュの切り替え設定。
- **Fields:** `List<LODLevel> Levels` (Distance, MeshID)

---

## 4. Animation Components (Planned) 🚧

### `Skeleton`
ボーン階層データ。
- **Fields:** `List<Matrix4x4> BoneMatrices`, `List<string> BoneNames`

### `SkinnedMeshRenderer`
ボーンウェイト付きメッシュ描画。
- **Fields:** `Mesh* mesh`, `Skeleton* skeleton`

### `Animator`
アニメーションステートマシン管理。
- **Fields:** `AnimatorController* controller`, `Map<string, float> Parameters`

---

## 5. Physics Components (Planned - Jolt Physics) 🚧

### Colliders
形状定義コンポーネント。
- `BoxCollider`: `Vector3 Size`, `Vector3 Center`
- `SphereCollider`: `float Radius`, `Vector3 Center`
- `CapsuleCollider`: `float Height`, `float Radius`
- `MeshCollider`: `Mesh* collisionMesh`

### `Rigidbody`
物理挙動パラメータ。
- **Fields:** `float Mass`, `float Drag`, `bool IsKinematic`, `float GravityScale`

### `CharacterController`
人型キャラクター制御用。
- **Fields:** `float StepOffset`, `float SlopeLimit`

### `Joint`
物理ボディ間の拘束。
- **Fields:** `Type` (Hinge/Fixed/Ball), `Entity ConnectedBody`

---

## 6. Audio Components (Planned) 🚧

### `AudioSource`
音源。
- **Fields:** `AudioClip* clip`, `float Volume`, `float Pitch`, `bool Loop`, `float SpatialBlend` (2D/3D)

### `AudioListener`
音を聞く位置（通常はカメラ）。
- **Fields:** (Tag Component)

### `ReverbZone`
環境リバーブ設定。
- **Fields:** `ReverbPreset Preset`, `float MaxDistance`

---

## 7. UI Components (Planned - ECS UI) 🚧

### `RectTransform`
2Dレイアウト用トランスフォーム。
- **Fields:** `Vector2 AnchorMin`, `Vector2 AnchorMax`, `Vector2 Pivot`, `Vector2 SizeDelta`

### `Canvas`
UI描画ルート。
- **Fields:** `int SortOrder`, `RenderMode` (Overlay/Camera)

### `Image`, `Text`, `Button`
標準UI要素。