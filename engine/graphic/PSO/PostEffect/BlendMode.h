#pragma once
//ブレンドモード
enum BlendMode
{
	//!<ブレンドなし
	kBlendModeNone,
	//!<通常アルファブレンド
	kBlendModeNormal,
	//!<加算アルファブレンド
	kBlendModeAdd,
	//!<減算アルファブレンド
	kBlendModeSubtract,
	//!<乗算アルファブレンド
	kBlendModeMultily,
	//!<スクリーン
	kBlendModeScreen,
	//利用してはいけない
	kCountBlendMode,
};