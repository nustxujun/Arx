#pragma once 

#include "ArxEntity.h"


class ARXRUNTIME_API ArxSystem: public ArxEntity
{
public:
    using ArxEntity::ArxEntity;

    virtual void PreUpdate(int FrameId) {};
    virtual void Update(int FrameId) {};
    virtual void PostUpdate(int FrameId) {};
    
};

