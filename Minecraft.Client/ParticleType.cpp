
#include "stdafx.h"
#include "ParticleType.h"

ParticleType::ParticleType(const std::string& name, int id, bool overrideLimiter, int paramCount)
{
    this->name            = name;           
    this->id              = id;             
    this->overrideLimiter = overrideLimiter; 
    this->paramCount      = paramCount;    
}

int ParticleType::getId() const
{
    return id; 
}

bool ParticleType::getOverrideLimiter() const
{
    return overrideLimiter; 
}

int ParticleType::getParamCount() const
{
    return paramCount;
}


const ParticleType* ParticleType::getDefault()
{
   
    return nullptr;
}

const ParticleType* ParticleType::byId(int searchId)
{
 
    return nullptr;
}