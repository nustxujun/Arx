#pragma once 

#include "CoreMinimal.h"
#include "ArxFixedPoint.h"

#if defined(_MSC_VER) && !defined(__clang__)
#define UNIQUE_FUNCTION_ID __FUNCSIG__
#else
#define UNIQUE_FUNCTION_ID __PRETTY_FUNCTION__
#endif

#define CHECK_DETER() check(GetWorld().IsDeterministic());

using ArxEntityId = uint32;
using ArxPlayerId = uint32;
using ArxFixed64 = Arx::FixedPoint64<32>;


#define NON_PLAYER_CONTROL -1
#define INVALID_ENTITY_ID -1

class ArxEntity;
class ArxWorld;


DECLARE_STATS_GROUP(TEXT("ArxGroup"), STATGROUP_ArxGroup, STATCAT_Advanced);

#include "ArxSerializer.h"

inline ArxBasicSerializer& operator << (ArxBasicSerializer& Ser, ArxFixed64& Val)
{
    ArxFixed64::fixed_raw Raw ;
    if (Ser.IsSaving())
    {
        Raw = Val.raw_value();
        Ser << Raw;
    }
    else
    {
        Ser << Raw;
        Val = ArxFixed64::from_raw(Raw);
    }
    return Ser;
}

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

template<class T, bool UniqueValue = false, class Pred = TLess<T>>
class TOrderedArray
{
public:
    int Insert(T Value)
    {
        int Index = LowerBound(Value);
        if (UniqueValue && Index < Array.Num() && Array[Index] == Value)
        {
            return Index;
        }
        
        Array.Insert(MoveTemp(Value), Index);
        return Index;
    }

    void Remove(const T& Value, bool bShrinking = false)
    {
        if (UniqueValue)
        {
            auto Index = LowerBound(Value);
            if (Index < Array.Num())
            {
                Array.RemoveAt(Index,1, bShrinking);
            }
        }
        else
        {
            auto Begin = LowerBound(Value);
            auto End = UpperBound(Value);
            auto Count = End - Begin;
            if (Count > 0)
                Array.RemoveAt(Begin, Count, bShrinking);
        }

    }

    int LowerBound(const T& Val)
    {
       return Algo::LowerBoundBy(Array, Val, FIdentityFunctor(), Pred());
    }

    int UpperBound(const T& Val)
    {
        return Algo::UpperBoundBy(Array, Val, FIdentityFunctor(), Pred());
    }

    int Find(const T& Val)
    {
        return Algo::BinarySearchBy(Array, Val, FIdentityFunctor(), Pred());
    }

    T& operator[](int Index)
    {
        return Array[Index];
    }


    TArray<T>& GetArray()
    {
        return Array;
    }

    const TArray<T>& GetArray() const
    {
        return Array;
    }
private:
    TArray<T> Array;
};

template<class T, bool UniqueValue = false, class Pred = TLess<T>>
inline ArxBasicSerializer& operator << (ArxBasicSerializer& Ser, TOrderedArray<T, UniqueValue, Pred>& Array)
{
    Ser << Array.GetArray();
    return Ser;
}


template<class T, bool UniqueValue = false, class Pred = TLess<T>>
inline FString LexToString(const TOrderedArray<T, UniqueValue, Pred>& Array)
{
    FString Content;
    for (auto& Item : Array.GetArray())
    {
        Content += FString::Printf(TEXT("\t%s,\n"), *LexToString(Item));
    }

    return FString::Printf(TEXT("{\n%s\n}\n"), *Content);
}



class ArxConstants
{
public:
    static constexpr ArxFixed64 TimeStep = 1.0 / 15.0;
    static constexpr int NumPhysicsStep = 4;
    static constexpr int VerificationCycle = 1; // frame
};