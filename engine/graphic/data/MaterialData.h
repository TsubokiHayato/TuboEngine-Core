#pragma once
#include <cstdint>
#include <string>

namespace TuboEngine {
// マテリアルデータ
struct MaterialData {
	// テクスチャファイルパス
	std::string textureFilePath;
	// テクスチャ番号
	uint32_t textureIndex = 0;
};
} // namespace TuboEngine