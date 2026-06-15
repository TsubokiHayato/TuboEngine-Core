#pragma once
#include "IParticleEmitter.h"

// Minimal emitter without gravity/drag. Uses billboarded quad.
class DefaultEmitter : public IParticleEmitter {
public:
    void Initialize(const ParticlePreset& preset) override {
        // Start from given preset and enforce minimal physics
        ParticlePreset p = preset;
        p.gravity = {0.0f, 0.0f, 0.0f};
        p.drag = 0.0f;
        // Visibility-safe defaults
        if (p.texture.empty()) { p.texture = "particle.png"; }
        p.billboard = true;
        p.simulateInWorldSpace = true;
        if (p.scaleStart.x == 0 && p.scaleStart.y == 0 && p.scaleStart.z == 0) {
            p.scaleStart = {0.25f,0.25f,0.25f}; // modest default size
        }
        if (p.scaleEnd.x == 0 && p.scaleEnd.y == 0 && p.scaleEnd.z == 0) {
            p.scaleEnd = p.scaleStart;
        }
        if (p.colorStart.w <= 0.0f) { p.colorStart.w = 1.0f; }
        // Assign and run base initialization to load texture and setup buffers
        IParticleEmitter::Initialize(p);
        // Do NOT multiply scale; keep as-is to avoid over-large quads
    }

    void BuildGeometry(std::vector<TuboEngine::VertexData>& out) override {
        // Unit quad geometry (same scale behavior as PrimitiveEmitter)
        out.clear();
        out.reserve(6);
		TuboEngine::VertexData v0{}, v1{}, v2{}, v3{};
        v0.position = { 1.0f,  1.0f, 0.0f, 1.0f}; v0.texcoord = {0.0f, 0.0f}; v0.normal = {0,0,1};
        v1.position = {-1.0f,  1.0f, 0.0f, 1.0f}; v1.texcoord = {1.0f, 0.0f}; v1.normal = {0,0,1};
        v2.position = { 1.0f, -1.0f, 0.0f, 1.0f}; v2.texcoord = {0.0f, 1.0f}; v2.normal = {0,0,1};
        v3.position = {-1.0f, -1.0f, 0.0f, 1.0f}; v3.texcoord = {1.0f, 1.0f}; v3.normal = {0,0,1};
        out.push_back(v0); out.push_back(v1); out.push_back(v2);
        out.push_back(v2); out.push_back(v1); out.push_back(v3);
    }

    ParticleInfo GenerateParticle() override {
        ParticleInfo info{};
        const auto& p = preset_;
        // RNG helpers
        auto uni = [&](float a, float b){ std::uniform_real_distribution<float> d(a, b); return d(rng_); };
        // Transform
        info.transform.scale = p.scaleStart;
        info.transform.rotate = {0.0f, 0.0f, 0.0f};
        const float rx = uni(p.posMin.x, p.posMax.x);
        const float ry = uni(p.posMin.y, p.posMax.y);
        const float rz = uni(p.posMin.z, p.posMax.z);
		info.transform.translate = p.center + TuboEngine::Math::Vector3{rx, ry, rz};
        // Velocity
        info.velocity = {
            uni(p.velMin.x, p.velMax.x),
            uni(p.velMin.y, p.velMax.y),
            uni(p.velMin.z, p.velMax.z)
        };
        // Lifetime
        info.lifeTime = uni(p.lifeMin, p.lifeMax);
        info.currentTime = 0.0f;
        // Color
        info.color = p.colorStart;
        return info;
    }
};
