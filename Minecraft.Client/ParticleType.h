
#pragma once
#include <string>

class ParticleType
{
private:
    std::string name;       
    int         id;         
    bool        overrideLimiter; 
    int         paramCount; 
    

public:
    ParticleType(const std::string& name, int id, bool overrideLimiter, int paramCount);

    int         getId()              const;
    bool        getOverrideLimiter() const;
    int         getParamCount()      const;

    static const ParticleType* byId(int id);
    static const ParticleType* getDefault();
};