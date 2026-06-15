#pragma once
#include <string>

namespace TuboEngine {
namespace StringUtility {
/// <summary>
/// ワイド文字列をマルチバイト文字列に変換
/// </summary>
/// <param name="str">変換したいワイド文字列</param>
/// <returns>変換されたマルチバイト文字列</returns>
std::string ConvertString(const std::wstring& str);

/// <summary>
/// マルチバイト文字列をワイド文字列に変換
/// </summary>
/// <param name="str">変換したいマルチバイト文字列</param>
/// <returns>変換されたワイド文字列</returns>
std::wstring ConvertString(const std::string& str);
}; // namespace StringUtility

} // namespace TuboEngine