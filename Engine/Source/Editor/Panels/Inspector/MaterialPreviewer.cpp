/*****************************************************************//**
 * @file	MaterialPreviewer.cpp
 * @brief	MaterialPreviewerの実装。
 *
 * @details
 *
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 *********************************************************************/

#include "MaterialPreviewer.h"
#include "Runtime/Application.h"

namespace Span
{
	MaterialPreviewer::~MaterialPreviewer()
	{
		Shutdown();
	}

	bool MaterialPreviewer::Initialize(ID3D12Device* device)
	{
		if (m_IsInitialized) return true;

		// 256x256 の高解像度プレビューターゲット
		if (!m_RenderTarget.Initialize(device, 256, 256, DXGI_FORMAT_R8G8B8A8_UNORM)) return false;

		GenerateSphereMesh(device);
		m_IsInitialized = true;
		return true;
	}

	void MaterialPreviewer::Shutdown()
	{
		m_RenderTarget.Shutdown();
		SAFE_DELETE(m_SphereMesh);
		m_IsInitialized = false;
	}

	void MaterialPreviewer::Render(ID3D12GraphicsCommandList* cmdList, Renderer* renderer, Material* material)
	{
		if (!m_IsInitialized || !material || !renderer || !cmdList) return;

		// --- 1. 元の状態（カメラ行列）を保存 ---
		Matrix4x4 oldView = renderer->GetViewMatrix();
		Matrix4x4 oldProj = renderer->GetProjectionMatrix();

		// --- 2. プレビュー用レンダーターゲットの設定 ---
		m_RenderTarget.TransitionToRenderTarget(cmdList);

		D3D12_CPU_DESCRIPTOR_HANDLE rtv = m_RenderTarget.GetRTV();
		D3D12_CPU_DESCRIPTOR_HANDLE dsv = m_RenderTarget.GetDSV();
		cmdList->OMSetRenderTargets(1, &rtv, FALSE, &dsv);

		const float clearColor[] = { 0.15f, 0.15f, 0.15f, 1.0f };
		cmdList->ClearRenderTargetView(rtv, clearColor, 0, nullptr);
		cmdList->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

		D3D12_VIEWPORT vp = { 0.0f, 0.0f, (float)m_RenderTarget.GetWidth(), (float)m_RenderTarget.GetHeight(), 0.0f, 1.0f };
		D3D12_RECT scissor = { 0, 0, (long)m_RenderTarget.GetWidth(), (long)m_RenderTarget.GetHeight() };
		cmdList->RSSetViewports(1, &vp);
		cmdList->RSSetScissorRects(1, &scissor);

		// --- 3. プレビュー専用カメラで球体を描画 ---
		Matrix4x4 proj = Matrix4x4::PerspectiveFovLH(45.0f, 1.0f, 0.1f, 100.0f);
		Matrix4x4 view = Matrix4x4::LookAtLH(Vector3(0, 0, -3.0f), Vector3(0, 0, 0), Vector3(0, 1, 0));
		renderer->SetCamera(view, proj);

		// 球体を描画する前に、エンジン用のヒープとグローバルリソースを再バインドする
		renderer->BindGlobalResources();

		Matrix4x4 world = Matrix4x4::Identity();
		renderer->DrawMesh(m_SphereMesh, material, world);

		// --- 4. 状態を元に戻す ---

		// 4-a. SRVとして読めるように状態遷移
		m_RenderTarget.TransitionToShaderResource(cmdList);

		// 4-b. カメラを元のメインカメラに戻す
		renderer->SetCamera(oldView, oldProj);

		// 4-c. 描画先をメイン画面（バックバッファ）に戻す
		if (renderer->GetContext())
		{
			renderer->GetContext()->SetRenderTargetToBackBuffer(cmdList);
		}

		// 4-d. ビューポートとシザーを元のメイン画面のサイズに戻す
		float mainWidth = (float)Application::Get().GetWindow().GetWidth();
		float mainHeight = (float)Application::Get().GetWindow().GetHeight();
		D3D12_VIEWPORT mainVp = { 0.0f, 0.0f, mainWidth, mainHeight, 0.0f, 1.0f };
		D3D12_RECT mainScissor = { 0, 0, (long)mainWidth, (long)mainHeight };
		cmdList->RSSetViewports(1, &mainVp);
		cmdList->RSSetScissorRects(1, &mainScissor);
	}

	void MaterialPreviewer::GenerateSphereMesh(ID3D12Device* device)
	{
		m_SphereMesh = Mesh::CreateSphere(device, 64, 64);
	}
}
