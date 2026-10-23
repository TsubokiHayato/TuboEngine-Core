#pragma once
#include "IParticleEmitter.h"
#include "JsonManager.h"

// =============================================================================
//  ParticlePreset ⇔ JSON の対応表（保存・読み込み・Undo のすべてがこれを使う）
//
//  ★ ParticlePreset に項目を増やしたら、ここに X("JSONのキー", メンバ名) を1行足すだけ。
//    保存と読み込みが同じ一覧から作られるので、片方だけ書き忘れることはない。
//    キーが無い古い JSON は、その項目だけ ParticlePreset の初期値になる。
//
//  エミッターの種類（"type"）や種類固有の値（"spinSpeedY" など）は ParticleManager 側で付け足す。
//  _emitAccum（内部用の蓄積時間）は保存しない。
// =============================================================================
#define TUBO_PARTICLE_PRESET_FIELDS(X)                                                                      \
	X("name", name)                                                                                         \
	X("texture", texture)                                                                                   \
	X("maxInstances", maxInstances)                                                                         \
	X("billboard", billboard)                                                                               \
	X("center", center)                                                                                     \
	X("posMin", posMin)                                                                                     \
	X("posMax", posMax)                                                                                     \
	X("velMin", velMin)                                                                                     \
	X("velMax", velMax)                                                                                     \
	X("scaleMin", scaleMin)                                                                                 \
	X("scaleMax", scaleMax)                                                                                 \
	X("colMin", colMin)                                                                                     \
	X("colMax", colMax)                                                                                     \
	X("lifeMin", lifeMin)                                                                                   \
	X("lifeMax", lifeMax)                                                                                   \
	X("autoEmit", autoEmit)                                                                                 \
	X("emitRate", emitRate)                                                                                 \
	X("burstCount", burstCount)                                                                             \
	X("gravity", gravity)                                                                                   \
	X("drag", drag)                                                                                         \
	X("rotSpeedRangeZ", rotSpeedRangeZ)                                                                     \
	X("initialRotRangeZ", initialRotRangeZ)                                                                 \
	X("scaleStart", scaleStart)                                                                             \
	X("scaleEnd", scaleEnd)                                                                                 \
	X("colorStart", colorStart)                                                                             \
	X("colorEnd", colorEnd)                                                                                 \
	X("blendModeOverride", blendModeOverride)                                                               \
	X("simulateInWorldSpace", simulateInWorldSpace)                                                         \
	X("emitterPos", emitterTransform.translate)

TUBO_JSON_DEFINE(ParticlePreset, TUBO_PARTICLE_PRESET_FIELDS)
