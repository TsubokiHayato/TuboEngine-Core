#pragma once
#include "Vector4.h"

namespace TuboEngine::Math {

struct Matrix4x4 {
    float m[4][4];

    // *= 演算子のオーバーロード
    Matrix4x4& operator*=(const Matrix4x4& other) {
        Matrix4x4 result = {};
        for (int i = 0; i < 4; ++i) {
            for (int j = 0; j < 4; ++j) {
                for (int k = 0; k < 4; ++k) {
                    result.m[i][j] += m[i][k] * other.m[k][j];
                }
            }
        }
        *this = result;
        return *this;
    }

    // * 演算子のオーバーロード
    friend Matrix4x4 operator*(const Matrix4x4& lhs, const Matrix4x4& rhs) {
        Matrix4x4 result = lhs;
        result *= rhs;
        return result;
    }
};

} // namespace TuboEngine::Math
