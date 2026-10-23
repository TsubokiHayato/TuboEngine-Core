#pragma once
#include "Vector2.h"
#include "Vector3.h"
#include "Vector4.h"
#include <externals/nlohmann/json.hpp>
#include <string>

// =============================================================================
//  JsonManager ── JSON ファイルの読み書きをまとめたもの
//
//  ・ファイルの読み込み / 保存（失敗したら Console にログが出る。保存は書き込み途中で落ちても壊れない）
//  ・Vector2 / Vector3 / Vector4 は j["pos"] = v; / Read(j, "pos", v); でそのまま使える（[x, y, z] の配列になる）
//  ・Read() はキーが無い・型が違うときは値をそのまま残す（＝変数の初期値が既定値になる）
//
//  使い方:
//    nlohmann::json j;
//    if (TuboEngine::JsonManager::Load("Resources/Player.json", j)) {
//        TuboEngine::JsonManager::Read(j, "speed", speed_);
//        TuboEngine::JsonManager::Read(j, "position", position_);
//    }
//
//    nlohmann::json out;
//    out["speed"] = speed_;
//    out["position"] = position_;
//    TuboEngine::JsonManager::Save("Resources/Player.json", out);
//
//  構造体を丸ごと扱うときは、項目一覧を1か所に書く（保存と読み込みで項目がずれない）:
//    #define PLAYER_PARAM_FIELDS(X) X("speed", speed) X("hp", hp) X("position", position)
//    TUBO_JSON_DEFINE(PlayerParam, PLAYER_PARAM_FIELDS)   // ← to_json / from_json が作られる
//    ※ 構造体と同じ名前空間に書くこと
//
//    PlayerParam param;
//    TuboEngine::JsonManager::LoadObject("Resources/Player.json", param);
//    TuboEngine::JsonManager::SaveObject("Resources/Player.json", param);
// =============================================================================

namespace TuboEngine {

class JsonManager {
public:
	using Json = nlohmann::json;

	/// <summary>
	/// ファイルを読み込んで out に入れる。
	/// 失敗（ファイルが無い・JSON として読めない）したら false。
	/// </summary>
	/// <param name="logIfMissing">ファイルが無い時もログを出すか（「無ければ既定値」の設定ファイルなどは false）</param>
	static bool Load(const std::string& path, Json& out, bool logIfMissing = true);

	/// <summary>
	/// j をファイルに保存する。フォルダが無ければ作る。
	/// 一時ファイルに書いてから置き換えるので、途中で落ちても元のファイルは壊れない。
	/// </summary>
	/// <param name="indent">字下げの幅（-1 で1行に詰める）</param>
	static bool Save(const std::string& path, const Json& j, int indent = 2);

	/// <summary>
	/// 文字列を JSON として読む（Undo 用スナップショットなど）。失敗したら false。
	/// </summary>
	static bool Parse(const std::string& text, Json& out);

	/// <summary>
	/// 相対パスを実際に使うフルパスにする。
	/// 作業フォルダにあればそれを、無ければ exe と同じフォルダにあるものを使う（どちらにも無ければ作業フォルダ）。
	/// </summary>
	static std::string ResolvePath(const std::string& path);

	/// <summary>
	/// j[key] を value に読み込む。キーが無い・型が違う時は value を変えずに false を返す。
	/// </summary>
	template<typename T> static bool Read(const Json& j, const char* key, T& value) {
		if (!j.is_object()) {
			return false;
		}
		auto it = j.find(key);
		if (it == j.end() || it->is_null()) {
			return false;
		}
		try {
			T tmp = value; // 失敗した時に value を壊さないよう、コピーに読んでから入れ替える
			it->get_to(tmp);
			value = std::move(tmp);
			return true;
		} catch (const std::exception&) {
			return false;
		}
	}

	/// <summary>
	/// ファイルを読み込んで value に入れる（TUBO_JSON_DEFINE した型など、from_json がある型）。
	/// </summary>
	template<typename T> static bool LoadObject(const std::string& path, T& value, bool logIfMissing = true) {
		Json j;
		if (!Load(path, j, logIfMissing)) {
			return false;
		}
		try {
			T tmp = value;
			j.get_to(tmp);
			value = std::move(tmp);
			return true;
		} catch (const std::exception& e) {
			LogError(path, e.what());
			return false;
		}
	}

	/// <summary>
	/// value をファイルに保存する（TUBO_JSON_DEFINE した型など、to_json がある型）。
	/// </summary>
	template<typename T> static bool SaveObject(const std::string& path, const T& value, int indent = 2) { return Save(path, Json(value), indent); }

private:
	static void LogError(const std::string& path, const std::string& reason);
};

namespace Math {
// --- Vector ⇔ JSON 配列の変換（nlohmann が名前空間から自動で見つける）---
// 要素数が足りない配列・配列以外のときは値を変えない（JsonManager::Read の既定値が残る）
inline void to_json(nlohmann::json& j, const Vector2& v) { j = nlohmann::json::array({v.x, v.y}); }
inline void to_json(nlohmann::json& j, const Vector3& v) { j = nlohmann::json::array({v.x, v.y, v.z}); }
inline void to_json(nlohmann::json& j, const Vector4& v) { j = nlohmann::json::array({v.x, v.y, v.z, v.w}); }
inline void from_json(const nlohmann::json& j, Vector2& v) {
	if (j.is_array() && j.size() >= 2) {
		v = {j[0].get<float>(), j[1].get<float>()};
	}
}
inline void from_json(const nlohmann::json& j, Vector3& v) {
	if (j.is_array() && j.size() >= 3) {
		v = {j[0].get<float>(), j[1].get<float>(), j[2].get<float>()};
	}
}
inline void from_json(const nlohmann::json& j, Vector4& v) {
	if (j.is_array() && j.size() >= 4) {
		v = {j[0].get<float>(), j[1].get<float>(), j[2].get<float>(), j[3].get<float>()};
	}
}
} // namespace Math

} // namespace TuboEngine

// -----------------------------------------------------------------------------
// TUBO_JSON_DEFINE(型, 項目一覧マクロ)
//   項目一覧マクロは X("JSONのキー", メンバ名) を並べたもの。
//   to_json は全項目を書き、from_json はある項目だけ読む（無い項目は初期値のまま）。
//   保存と読み込みが同じ一覧から作られるので、片方だけ書き忘れることがない。
// -----------------------------------------------------------------------------
#define TUBO_JSON_DETAIL_WRITE(key, member) j[key] = value.member;
#define TUBO_JSON_DETAIL_READ(key, member) ::TuboEngine::JsonManager::Read(j, key, value.member);
#define TUBO_JSON_DEFINE(Type, FIELDS)                                                   \
	inline void to_json(nlohmann::json& j, const Type& value) {                          \
		j = nlohmann::json::object();                                                    \
		FIELDS(TUBO_JSON_DETAIL_WRITE)                                                   \
	}                                                                                    \
	inline void from_json(const nlohmann::json& j, Type& value) { FIELDS(TUBO_JSON_DETAIL_READ) }
