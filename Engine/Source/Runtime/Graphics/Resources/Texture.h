#pragma once
#include "Core/CoreMinimal.h"

namespace Span
{
    class Texture
    {
    public:
        Texture();
        ~Texture();

        // 画像ファイルからテクスチャを作成
        bool Initialize(ID3D12Device* device, ID3D12CommandQueue* commandQueue, const std::string& filepath);

        // 終了処理
        void Shutdown();

        // ディスクリプタヒープ上の位置 (CPUハンドル) を返す
        D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandle() const { return srvHandleCPU; }

        // シェーダーリソースビュー (SRV) の作成
        ID3D12DescriptorHeap* GetSRVHeap() const { return srvHeap.Get(); }

    private:
        // 画像データをGPUバッファへアップロードするヘルパー関数
        bool UploadTexture(ID3D12Device* device, ID3D12CommandQueue* commandQueue,
            const void* initialData, uint64_t width, uint64_t height, uint64_t bytesPerPixel);

    private:
        ComPtr<ID3D12Resource> resource;       // テクスチャ本体 (VRAM)
        ComPtr<ID3D12Resource> uploadBuffer;   // アップロード用の一時バッファ (Upload Heap)
        ComPtr<ID3D12DescriptorHeap> srvHeap;  // SRV用デスクリプタヒープ (このテクスチャ専用)

        D3D12_CPU_DESCRIPTOR_HANDLE srvHandleCPU = {};

        uint32_t width = 0;
        uint32_t height = 0;
    };
}