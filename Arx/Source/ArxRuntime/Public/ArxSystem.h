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

template<class T>
class ArxBasicSystem : public ArxSystem
{
public:
    using ArxSystem::ArxSystem;

    static TFunction<ArxEntity* (ArxWorld&, ArxEntityId)> GetFactory()
    {
        return [](ArxWorld& InWorld, ArxEntityId Id)->ArxEntity*
        {
            auto Inst = new T(InWorld, Id);
            return Inst;
        };
    }

    static FName GetTypeName()
    {
        return TypeName;
    }

    virtual FName GetClassName() override
    {
        return GetTypeName();
    }

private:
    static const FName TypeName;

};

template<class T>
const FName ArxBasicSystem<T>::TypeName = []() {
    auto Name = ArxTypeName<T>();
    ArxEntity::GetFactories().Add(Name, T::GetFactory());
    return Name;
}();

