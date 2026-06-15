#pragma once
#include <cmath>
#include <numbers>

namespace TuboEngine::Math {

class Quaternion {
public:
    float x, y, z, w;

    // デフォルトコンストラクタ（単位クォータニオン）
    Quaternion() : x(0), y(0), z(0), w(1) {}

    // パラメータ付きコンストラクタ
    Quaternion(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}

     Vector3 ToEuler() const {
		Vector3 euler;

		// Yaw (Z軸回り)
		float siny_cosp = 2.0f * (w * z + x * y);
		float cosy_cosp = 1.0f - 2.0f * (y * y + z * z);
		euler.z = std::atan2(siny_cosp, cosy_cosp);

		// Pitch (Y軸回り)
		float sinp = 2.0f * (w * y - z * x);
		if (std::abs(sinp) >= 1)
			euler.y = std::copysign(std::numbers::pi_v<float> / 2, sinp); // 90度
		else
			euler.y = std::asin(sinp);

		// Roll (X軸回り)
		float sinr_cosp = 2.0f * (w * x + y * z);
		float cosr_cosp = 1.0f - 2.0f * (x * x + y * y);
		euler.x = std::atan2(sinr_cosp, cosr_cosp);

		return euler;
	}

    // 球面線形補間（Slerp）
    static Quaternion Slerp(const Quaternion& a, const Quaternion& b, float t) {
        // 内積を計算
        float dot = a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
        // クォータニオンの向きを揃える
        Quaternion end = b;
        if (dot < 0.0f) {
            dot = -dot;
            end.x = -b.x;
            end.y = -b.y;
            end.z = -b.z;
            end.w = -b.w;
        }
        // 線形補間で十分近い場合
        const float DOT_THRESHOLD = 0.9995f;
        if (dot > DOT_THRESHOLD) {
            // 線形補間
            Quaternion result(
                a.x + t * (end.x - a.x),
                a.y + t * (end.y - a.y),
                a.z + t * (end.z - a.z),
                a.w + t * (end.w - a.w)
            );
            // 正規化
            float len = std::sqrt(result.x * result.x + result.y * result.y + result.z * result.z + result.w * result.w);
            result.x /= len;
            result.y /= len;
            result.z /= len;
            result.w /= len;
            return result;
        }
        // 球面線形補間
        float theta_0 = std::acos(dot);      // 角度
        float theta = theta_0 * t;           // tに応じた角度
        float sin_theta = std::sin(theta);   // tに応じたSin
        float sin_theta_0 = std::sin(theta_0); // 元のSin
        float s0 = std::cos(theta) - dot * sin_theta / sin_theta_0;
        float s1 = sin_theta / sin_theta_0;
        Quaternion result(
            (a.x * s0) + (end.x * s1),
            (a.y * s0) + (end.y * s1),
            (a.z * s0) + (end.z * s1),
            (a.w * s0) + (end.w * s1)
        );
        // 正規化
        float len = std::sqrt(result.x * result.x + result.y * result.y + result.z * result.z + result.w * result.w);
        result.x /= len;
        result.y /= len;
        result.z /= len;
        result.w /= len;
        return result;
    }
};

} // namespace TuboEngine::Math
