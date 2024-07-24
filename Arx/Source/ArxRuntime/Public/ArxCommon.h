#pragma once 

#include "CoreMinimal.h"

#if defined(_MSC_VER) && !defined(__clang__)
#define UNIQUE_FUNCTION_ID __FUNCSIG__
#else
#define UNIQUE_FUNCTION_ID __PRETTY_FUNCTION__
#endif

#define CHECK_DETER() check(GetWorld().IsDeterministic());

using ArxEntityId = uint32;
using ArxPlayerId = uint32;

#define NON_PLAYER_CONTROL -1
#define INVALID_ENTITY_ID -1

class ArxEntity;
class ArxWorld;


DECLARE_STATS_GROUP(TEXT("ArxGroup"), STATGROUP_ArxGroup, STATCAT_Advanced);

#include "ArxSerializer.h"

template<class T>
inline uint64 ArxTypeId()
{
	static uint64 Id = (uint64)FCrc::MemCrc32(UNIQUE_FUNCTION_ID, sizeof(UNIQUE_FUNCTION_ID));
	return Id;
}

#include <regex>
#include <string>


template<class Type>
static FName ArxTypeName()
{
    const static FName TypeName = [
#if defined(_MSC_VER) && !defined(__clang__)
        FuncName = std::string(__FUNCSIG__)
#else
        FuncName = std::string(__PRETTY_FUNCTION__)
#endif 
    ]() ->FName
    {
#if defined(_MSC_VER) && !defined(__clang__)
        const std::regex Pattern("ArxTypeName<(class|struct) (.+)>");
#else
        const std::regex Pattern("\\[(Type =) (.+)\\]");
#endif
        std::smatch Result;
        auto bMatched = std::regex_search(FuncName, Result,Pattern);
        
        check(bMatched);
        auto Name = Result[2].str();
        Name = "[" + Name + "]";
        return Name.c_str();
    }();

    return TypeName;
}


template<class Key, class Value>
class TDoubleMap
{
public:
    void Add(const Key& K,const Value& V)
    {
        Fwd.Add(K,V);
        Bwd.Add(V,K);
    }

    Value* FindForward(const Key& K)
    {
        return Fwd.Find(K);
    }

    Key* FindBackward(const Value& V)
    {
        return Bwd.Find(V);
    }

    void Reset()
    {
        Fwd.Reset();
        Bwd.Reset();
    }
private:
    TMap<Key, Value> Fwd;
    TMap<Value, Key> Bwd;
};
