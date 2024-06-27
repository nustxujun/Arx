#pragma once 

#include "CoreMinimal.h"

template<class T>
class TRandomArray
{
public:
    void Set(int Index, T Value)
    {
        const auto Num = Array.Num();
        if (Index >= Num)
        {
            Array.SetNum(Index + 1, false);
            BitArray.SetNumUninitialized(Index + 1);
            BitArray.SetRange(Num, Index + 1 - Num, false);
        }
        BitArray[Index] = true;
        Array[Index] = MoveTemp(Value);
    }

    const T& Get(int Index)const
    {
        check(Has(Index));
        return Array[Index];
    }

    bool Has(int Index)const{
        return BitArray.Num() > Index && BitArray[Index]; 
    }

    void Remove(int Index)
    {
        if (BitArray.Num() > Index)
        {
            BitArray[Index] = false;
            Array[Index] = {};
        }
    }
    
private:
    TArray<T> Array;
    TBitArray<> BitArray;
};

