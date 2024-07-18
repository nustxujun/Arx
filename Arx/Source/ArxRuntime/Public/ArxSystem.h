#pragma once 

#include "ArxEntity.h"


class ARXRUNTIME_API ArxSystem: public ArxEntity
{
public:
    using ArxEntity::ArxEntity;

    virtual void PreUpdate() {};
    virtual void Update() {};
    virtual void PostUpdate() {};
    
};

