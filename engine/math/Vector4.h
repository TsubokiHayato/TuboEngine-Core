#pragma once
#include <cmath> // sqrt

namespace TuboEngine::Math {

struct Vector4
{
	float x;
	float y;
	float z;
	float w;

	// += 演算子のオーバーロード
	Vector4& operator+=(const Vector4& other) {
		this->x += other.x;
		this->y += other.y;
		this->z += other.z;
		this->w += other.w;
		return *this;
	}

	// + 演算子のオーバーロード
	friend Vector4 operator+(const Vector4& lhs, const Vector4& rhs) {
		Vector4 result = lhs;
		result += rhs;
		return result;
	}

	// -= 演算子のオーバーロード
	Vector4& operator-=(const Vector4& other) {
		this->x -= other.x;
		this->y -= other.y;
		this->z -= other.z;
		this->w -= other.w;
		return *this;
	}

	// - 演算子のオーバーロード
	friend Vector4 operator-(const Vector4& lhs, const Vector4& rhs) {
		Vector4 result = lhs;
		result -= rhs;
		return result;
	}

	// *= 演算子のオーバーロード
	Vector4& operator*=(float scalar) {
		this->x *= scalar;
		this->y *= scalar;
		this->z *= scalar;
		this->w *= scalar;
		return *this;
	}

	// * 演算子のオーバーロード
	friend Vector4 operator*(const Vector4& vec, float scalar) {
		Vector4 result = vec;
		result *= scalar;
		return result;
	}

	// * 演算子のオーバーロード
	friend Vector4 operator*(float scalar, const Vector4& vec) {
		return vec * scalar;
	}

	// /= 演算子のオーバーロード
	Vector4& operator/=(float scalar) {
		this->x /= scalar;
		this->y /= scalar;
		this->z /= scalar;
		this->w /= scalar;
		return *this;
	}

	// / 演算子のオーバーロード
	friend Vector4 operator/(const Vector4& vec, float scalar) {
		Vector4 result = vec;
		result /= scalar;
		return result;
	}

	// - 演算子のオーバーロード
	Vector4 operator-() const {
		return *this * -1;
	}

	// == 演算子のオーバーロード
	bool operator==(const Vector4& other) const {
		return this->x == other.x && this->y == other.y && this->z == other.z && this->w == other.w;
	}

	// != 演算子のオーバーロード
	bool operator!=(const Vector4& other) const {
		return !(*this == other);
	}

	// 単項+演算子のオーバーロード
	Vector4 operator+() const {
		return *this;
	}

	// ベクトルの長さを計算する関数
	float Length() const {
		return std::sqrt(LengthSquared());
	}

	// ベクトルの長さの2乗を計算する関数
	float LengthSquared() const {
		return x * x + y * y + z * z + w * w;
	}

	// ベクトルを正規化する関数
	void Normalize() {
		*this /= Length();
	}

	// ベクトルを正規化する関数
	Vector4 Normalized() const {
		return *this / Length();
	}

	// ベクトルの内積を計算する関数
	static float Dot(const Vector4& v1, const Vector4& v2) {
		return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z + v1.w * v2.w;
	}

	// ベクトルの外積を計算する関数
	static Vector4 Cross(const Vector4& v1, const Vector4& v2) {
		Vector4 result;
		result.x = v1.y * v2.z - v1.z * v2.y;
		result.y = v1.z * v2.x - v1.x * v2.z;
		result.z = v1.x * v2.y - v1.y * v2.x;
		result.w = 0;
		return result;
	}

	// ベクトルを正規化する関数
	static Vector4 Normalize(const Vector4& vec) {
		return vec / vec.Length();
	}


};

} // namespace TuboEngine::Math