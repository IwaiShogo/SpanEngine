#include "ModelLoader.h"
#include "Core/CoreMinimal.h" // ログ用

namespace Span
{
    std::vector<Mesh*> ModelLoader::Load(ID3D12Device* device, const std::string& filepath)
    {
        std::vector<Mesh*> meshes;
        Assimp::Importer importer;

        SPAN_LOG("Loading Model: %s", filepath.c_str());

        // 読み込みオプション
        // - Triangulate: 多角形を三角形に分割
        // - ConvertToLeftHanded: DirectX座標系(左手系)に変換
        // - GenNormals: 法線がない場合は計算
        const aiScene* scene = importer.ReadFile(filepath,
            aiProcess_Triangulate |
            aiProcess_ConvertToLeftHanded |
            aiProcess_GenNormals |
            aiProcess_CalcTangentSpace
        );

        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
        {
            SPAN_ERROR("Assimp Error: %s", importer.GetErrorString());
            return meshes;
        }

        // シーン内の全メッシュを処理
        for (unsigned int i = 0; i < scene->mNumMeshes; i++)
        {
            aiMesh* mesh = scene->mMeshes[i];
            meshes.push_back(ProcessMesh(device, mesh, scene));
        }

        SPAN_LOG("-> Loaded %d meshes.", meshes.size());
        return meshes;
    }

    Mesh* ModelLoader::ProcessMesh(ID3D12Device* device, aiMesh* mesh, const aiScene* scene)
    {
        std::vector<Vertex> vertices;

        // 現在の Mesh クラスはインデックスバッファを使っていないため、
        // インデックスを参照して頂点を「展開 (Flatten)」します。

        // ポリゴン（面）の数だけループ
        for (unsigned int i = 0; i < mesh->mNumFaces; i++)
        {
            aiFace face = mesh->mFaces[i];

            // 1つの面を構成する頂点（通常は3つ）
            for (unsigned int j = 0; j < face.mNumIndices; j++)
            {
                unsigned int index = face.mIndices[j];

                Vertex v;

                // 1. 位置 (Position)
                if (mesh->HasPositions())
                {
                    v.position.x = mesh->mVertices[index].x;
                    v.position.y = mesh->mVertices[index].y;
                    v.position.z = mesh->mVertices[index].z;
                }

                // 2. 法線 (Normal) -> デバッグ用に色として表示
                if (mesh->HasNormals())
                {
                    v.normal.x = mesh->mNormals[index].x;
                    v.normal.y = mesh->mNormals[index].y;
                    v.normal.z = mesh->mNormals[index].z;

                    // 法線を色に変換 (-1~1 -> 0~1)
                    v.color.x = (v.normal.x + 1.0f) * 0.5f;
                    v.color.y = (v.normal.y + 1.0f) * 0.5f;
                    v.color.z = (v.normal.z + 1.0f) * 0.5f;
                }
                else
                {
                    v.color = { 1.0f, 1.0f, 1.0f };
                }

                vertices.push_back(v);
            }
        }

        Mesh* newMesh = new Mesh();
        newMesh->Initialize(device, vertices);
        return newMesh;
    }
}