#pragma once
#include "Object3d.h"
#include "Camera.h"
#include "Vector3.h"
#include "Vector4.h"
#include "Matrix.h"
#include <memory>
#include <string>
#include <wrl/client.h>
#undef min
#undef max

/// @brief Object3d パイプラインで GPU インスタンシング描画するクラス
///
/// Object3d.VS.hlsl の useInstancing フラグと StructuredBuffer<InstanceData> を利用し、
/// Model::DrawInstanced() を呼ぶことで 1 DrawCall で N 個の 3D モデルを描画する。
///
/// Object3d の照明 (Phong / Blinn-Phong / PointLight 等) がそのまま使える。
///
/// 使い方:
///   InstancedMeshRenderer r;
///   r.Initialize("Resources/Model/Sphere/Sphere.obj", 1000, camera);
///   // 毎フレーム
///   r.Update(camera);
///   r.Draw(gpuAddr, count);   ← Object3dCommonDraw() パス内で呼ぶ
class InstancedMeshRenderer {
public:
    void Initialize(const std::string& modelPath, int maxInstances,
                    TuboEngine::Camera* camera);
    void Update(TuboEngine::Camera* camera);

    /// @param instanceDataGpuAddr  StructuredBuffer<InstanceData> の GPU アドレス
    ///                             InstanceData = { float4x4 WVP; float4x4 World; float4 Color; }
    /// @param instanceCount        描画インスタンス数
    void Draw(D3D12_GPU_VIRTUAL_ADDRESS instanceDataGpuAddr, uint32_t instanceCount);

    void Finalize();

    // ---- ライト設定 (Object3d に委譲) ----
    void SetLightType(int type)                                    { if (obj_) obj_->SetLightType(type); }
    void SetLightColor(const TuboEngine::Math::Vector4& col)      { if (obj_) obj_->SetLightColor(col); }
    void SetLightDirection(const TuboEngine::Math::Vector3& dir)  { if (obj_) obj_->SetLightDirection(dir); }
    void SetLightIntensity(float i)                               { if (obj_) obj_->SetLightIntensity(i); }

private:
    std::unique_ptr<TuboEngine::Object3d> obj_;  // ライト・カメラ・マテリアル管理用

    /// @brief useInstancing = 1 を格納する CBV (root param[10], VS b1)
    Microsoft::WRL::ComPtr<ID3D12Resource> commonBuf_;

    /// @brief root param[1] (VS b0 TransformMatrix) の dummy バッファ
    ///        useInstancing=1 のとき VS は読まないが、root param は必ずセットが必要
    Microsoft::WRL::ComPtr<ID3D12Resource> dummyTransformBuf_;
};
