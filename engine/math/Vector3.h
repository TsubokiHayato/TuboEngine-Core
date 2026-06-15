#pragma once
#include <algorithm>
#include <cmath>
#undef max
#undef min

namespace TuboEngine::Math {

struct Vector3
{
	float x;
	float y;
	float z;

	// += 演算子のオーバーロード
	Vector3& operator+=(const Vector3& other) {
		this->x += other.x;
		this->y += other.y;
		this->z += other.z;
		return *this;
	}

	// + 演算子のオーバーロード
	friend Vector3 operator+(const Vector3& lhs, const Vector3& rhs) {
		Vector3 result = lhs;
		result += rhs;
		return result;
	}

	// *= 演算子のオーバーロード
	Vector3& operator*=(float scalar) {
		this->x *= scalar;
		this->y *= scalar;
		this->z *= scalar;
		return *this;
	}

	// * 演算子のオーバーロード
	friend Vector3 operator*(const Vector3& vec, float scalar) {
		Vector3 result = vec;
		result *= scalar;
		return result;
	}

	// * 演算子のオーバーロード
	friend Vector3 operator*(float scalar, const Vector3& vec) {
		return vec * scalar;
	}

	// /= 演算子のオーバーロード
	Vector3& operator/=(float scalar) {
		this->x /= scalar;
		this->y /= scalar;
		this->z /= scalar;
		return *this;
	}

	// / 演算子のオーバーロード

	friend Vector3 operator/(const Vector3& vec, float scalar) {
		Vector3 result = vec;
		result /= scalar;
		return result;
	}

	// - 演算子のオーバーロード

	Vector3 operator-() const {
		return *this * -1;
	}

	// - 演算子のオーバーロード

	Vector3 operator-(const Vector3& other) const {
		Vector3 result = *this;
		result -= other;
		return result;
	}

	// -= 演算子のオーバーロード

	Vector3& operator-=(const Vector3& other) {
		this->x -= other.x;
		this->y -= other.y;
		this->z -= other.z;
		return *this;
	}

	// == 演算子のオーバーロード

	bool operator==(const Vector3& other) const {
		return this->x == other.x && this->y == other.y && this->z == other.z;
	}

	// != 演算子のオーバーロード

	bool operator!=(const Vector3& other) const {
		return !(*this == other);
	}

	// 単項+演算子のオーバーロード

	Vector3 operator+() const {
		return *this;
	}

	// ベクトルの長さを計算する関数

	float Length() const {
		return std::sqrt(LengthSquared());
	}

	// ベクトルの長さの2乗を計算する関数

	float LengthSquared() const {
		return x * x + y * y + z * z;
	}

	// ベクトルを正規化する関数

	void Normalize() {
		*this /= Length();
	}

	// ベクトルの内積を計算する関数

	static float Dot(const Vector3& v1, const Vector3& v2) {
		return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;
	}

	// ベクトルの外積を計算する関数

	static Vector3 Cross(const Vector3& v1, const Vector3& v2) {
		Vector3 result;
		result.x = v1.y * v2.z - v1.z * v2.y;
		result.y = v1.z * v2.x - v1.x * v2.z;
		result.z = v1.x * v2.y - v1.y * v2.x;
		return result;
	}

	// ベクトルを正規化する関数

	static Vector3 Normalize(const Vector3& vec) {
		return vec / vec.Length();
	}

	// 2つのベクトルの間の角度を計算する関数

	static float Angle(const Vector3& from, const Vector3& to) {
		float l = from.Length() * to.Length();
		if (l == 0) {
			return 0;
		}
		return std::acos(Dot(from, to) / l);
	}

	// 2つのベクトルの間の角度を計算する関数

	static float Angle(const Vector3& from, const Vector3& to, const Vector3& up) {
		Vector3 fromNormal = Normalize(from);
		Vector3 toNormal = Normalize(to);
		float angle = std::acos(Dot(fromNormal, toNormal));
		Vector3 cross = Cross(fromNormal, toNormal);
		if (Dot(up, cross) < 0) {
			angle = -angle;
		}
		return angle;
	}

	// 2つのベクトルの間の角度を計算する関数

	static float SignedAngle(const Vector3& from, const Vector3& to, const Vector3& axis) {
		float angle = Angle(from, to);
		Vector3 cross = Cross(from, to);
		if (Dot(axis, cross) < 0) {
			angle = -angle;
		}
		return angle;
	}

	// 2つのベクトルの間の角度を計算する関数

	static float SignedAngle(const Vector3& from, const Vector3& to, const Vector3& axis, const Vector3& up) {
		float angle = Angle(from, to, up);
		Vector3 cross = Cross(from, to);
		if (Dot(axis, cross) < 0) {
			angle = -angle;
		}
		return angle;
	}

	// 2つのベクトルの間の距離を計算する関数

	static float Distance(const Vector3& a, const Vector3& b) {
		return (a - b).Length();
	}

	// 2つのベクトルの間の距離の2乗を計算する関数

	static float DistanceSquared(const Vector3& a, const Vector3& b) {
		return (a - b).LengthSquared();
	}

	// 2つのベクトルの最小値を計算する関数

	static Vector3 Min(const Vector3& a, const Vector3& b) {
		return Vector3(std::min(a.x, b.x), std::min(a.y, b.y), std::min(a.z, b.z));
	}

	// 2つのベクトルの最大値を計算する関数

	static Vector3 Max(const Vector3& a, const Vector3& b) {
		return Vector3(std::max(a.x, b.x), std::max(a.y, b.y), std::max(a.z, b.z));
	}

	// 2つのベクトルの線形補間を計算する関数

	static Vector3 Lerp(const Vector3& a, const Vector3& b, float f) {
		return a + (b - a) * f;
	}

	// ゼロベクトルを取得する関数

	static Vector3 Zero() {
		return Vector3(0, 0, 0);
	}

	// X単位ベクトルを取得する関数

	static Vector3 UnitX() {
		return Vector3(1, 0, 0);
	}

	// Y単位ベクトルを取得する関数

	static Vector3 UnitY() {
		return Vector3(0, 1, 0);
	}

	// Z単位ベクトルを取得する関数

	static Vector3 UnitZ() {
		return Vector3(0, 0, 1);
	}

	// ワールド座標の前方ベクトルを取得する関数

	static Vector3 Forward() {
		return UnitZ();
	}

	// ワールド座標の上方ベクトルを取得する関数

	static Vector3 Up() {
		return UnitY();
	}

	// ワールド座標の右方向ベクトルを取得する関数

	static Vector3 Right() {
		return UnitX();
	}

};

} // namespace TuboEngine::Math