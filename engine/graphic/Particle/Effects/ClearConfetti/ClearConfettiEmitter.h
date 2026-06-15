#pragma once
#include "../Primitive/PrimitiveEmitter.h"

class ClearConfettiEmitter : public PrimitiveEmitter {
public:
    void Initialize(const ParticlePreset& preset) override {
        PrimitiveEmitter::Initialize(preset);
    }

protected:
    ParticleInfo GenerateParticle() override;
};