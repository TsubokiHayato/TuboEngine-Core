#pragma once

#include <algorithm>
#include <cmath>
#undef max
#undef min

namespace TuboEngine::Math {

struct Vector2
{
	float x;
	float y;

    // += 演算子のオーバーロード
    Vector2& operator+=(const Vector2& other) {
        this->x += other.x;
        this->y += other.y;
        return *this;
    }

	// -= 演算子のオーバーロード
	Vector2& operator-=(const Vector2& other) {
		this->x -= other.x;
		this->y -= other.y;
		return *this;
	}

	// *= 演算子のオーバーロード
	Vector2& operator*=(float scalar) {
		this->x *= scalar;
		this->y *= scalar;
		return *this;
	}

	// /= 演算子のオーバーロード

	Vector2& operator/=(float scalar) {
		this->x /= scalar;
		this->y /= scalar;
		return *this;
	}

	// + 演算子のオーバーロード

	Vector2 operator+(const Vector2& other) const {
		Vector2 result = *this;
		result += other;
		return result;
	}

	// - 演算子のオーバーロード

	Vector2 operator-(const Vector2& other) const {
		Vector2 result = *this;
		result -= other;
		return result;
	}

	// * 演算子のオーバーロード

	Vector2 operator*(float scalar) const {
		Vector2 result = *this;
		result *= scalar;
		return result;
	}

	// / 演算子のオーバーロード

	Vector2 operator/(float scalar) const {
		Vector2 result = *this;
		result /= scalar;
		return result;
	}

	// == 演算子のオーバーロード

	bool operator==(const Vector2& other) const {
		return this->x == other.x && this->y == other.y;
	}

	// != 演算子のオーバーロード

	bool operator!=(const Vector2& other) const {
		return !(*this == other);
	}

	// 単項+演算子のオーバーロード

	Vector2 operator+() const {
		return *this;
	}

	// 単項-演算子のオーバーロード

	Vector2 operator-() const {
		return *this * -1;
	}

	// ベクトルの長さを取得する関数

	float Length() const {
		return std::sqrt(x * x + y * y);
	}

	// ベクトルの長さの2乗を取得する関数

	float LengthSquared() const {
		return x * x + y * y;
	}

	// ベクトルを正規化する関数

	Vector2 Normalized() const {
		return *this / Length();
	}

	// ベクトルを正規化する関数

	void Normalize() {
		*this /= Length();
	}

	// ベクトルの内積を計算する関数

	static float Dot(const Vector2& v1, const Vector2& v2) {
		return v1.x * v2.x + v1.y * v2.y;
	}

	// ベクトルの外積を計算する関数

	static float Cross(const Vector2& v1, const Vector2& v2) {
		return v1.x * v2.y - v1.y * v2.x;
	}

	// 2つのベクトルの距離を計算する関数

	static float Distance(const Vector2& v1, const Vector2& v2) {
		return (v1 - v2).Length();
	}

	// 2つのベクトルの距離の2乗を計算する関数

	static float DistanceSquared(const Vector2& v1, const Vector2& v2) {
		return (v1 - v2).LengthSquared();
	}

	// 2つのベクトルの最小値を計算する関数

	static Vector2 Min(const Vector2& v1, const Vector2& v2) {
		return Vector2(std::min(v1.x, v2.x), std::min(v1.y, v2.y));
	}

	// 2つのベクトルの最大値を計算する関数

	static Vector2 Max(const Vector2& v1, const Vector2& v2) {
		return Vector2(std::max(v1.x, v2.x), std::max(v1.y, v2.y));
	}

	// 2つのベクトルの線形補間を計算する関数

	static Vector2 Lerp(const Vector2& a, const Vector2& b, float f) {
		return a + (b - a) * f;
	}

	// ゼロベクトルを取得する関数

	static Vector2 Zero() {
		return Vector2(0, 0);
	}

	// X単位ベクトルを取得する関数

	static Vector2 UnitX() {
		return Vector2(1, 0);
	}

	// Y単位ベクトルを取得する関数

	static Vector2 UnitY() {
		return Vector2(0, 1);
	}

};

} // namespace TuboEngine::Math
