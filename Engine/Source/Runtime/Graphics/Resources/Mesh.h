/*****************************************************************//**
 * @file	Mesh.h
 * @brief	3Dモデルの形状データ (頂点バッファ) 管理。
 *
 * @details
 *
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 *********************************************************************/

#pragma once
#include "Core/CoreMinimal.h"
#include "Core/Math/SpanMath.h"

namespace Span
{
	/**
	 * @struct	Vertex
	 * @brief	頂点フォーマット。
	 * @note	InputLayoutで指定するセマンティクスと一致している必要があります。
	 */
	struct Vertex
	{
		Vector3 position;	///< POSITION
		Vector3 normal;		///< NORMAL
		Vector2 uv;			///< TEXCOORD
	};

	/**
	 * @class	Mesh
	 * @brief	📦 頂点データをGPUメモリ (Vertex Buffer) に保持するクラス。
	 *
	 * @details
	 * - **VertexBufferView (VBV)** を通じて描画コマンドにバインドされます。
	 * - 現時点ではインデックスバッファを使用しない実装になっています (将来的に拡張推奨)。
	 */
	class Mesh
	{
	public:
		Mesh() = default;

		// Move Constructor
		Mesh(Mesh&&) noexcept = default;
		Mesh& operator=(Mesh&&) noexcept = default;

		// Copy Constructor
		Mesh(const Mesh&) = delete;
		Mesh& operator=(const Mesh&) = delete;

		~Mesh();

		/**
		 * @brief	頂点配列からメッシュを初期化します。
		 * @param	device D3D12デバイス
		 * @param	vertices 頂点データのリスト
		 */
		bool Initialize(ID3D12Device* device, const std::vector<Vertex>& vertices);

		void Shutdown();

		/**
		 * @brief	描画コマンドを発行します。
		 * @param	commandList 記録中のコマンドリスト
		 * @note	事前に `IASetPrimitiveTopology` 等の設定が必要です。
		 */
		void Draw(ID3D12GraphicsCommandList* commandList);

		// 🔨 Procedural Generation Helpers
		// ============================================================

		static Mesh* CreateCube(ID3D12Device* device);
		static Mesh* CreateSphere(ID3D12Device* device, int slices = 16, int stacks = 16);
		static Mesh* CreatePlane(ID3D12Device* device, float width = 10.0f, float depth = 10.0f);
		static Mesh* CreateCylinder(ID3D12Device* device, float radius = 0.5f, float height = 1.0f, int slices = 32);
		static Mesh* CreateCone(ID3D12Device* device, float radius = 0.5f, float height = 1.0f, int slices = 32);
		static Mesh* CreateTorus(ID3D12Device* device, float radius = 0.5f, float tubeRadius = 0.2f, int segments = 32, int tubeSegments = 16);
		static Mesh* CreateCapsule(ID3D12Device* device, float radius = 0.5f, float height = 2.0f, int slices = 32, int stacks = 16);

		/// @brief	パスの取得
		const std::string& GetPath() const { return m_FilePath; }

		/// @brief	パスの設定
		void SetPath(const std::string& path) { m_FilePath = path; }

	private:
		Microsoft::WRL::ComPtr<ID3D12Resource> vertexBuffer;
		D3D12_VERTEX_BUFFER_VIEW vertexBufferView = {};
		uint32 vertexCount = 0;

		std::string m_FilePath;
	};
}
