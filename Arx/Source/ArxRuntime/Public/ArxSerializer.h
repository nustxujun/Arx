#pragma once 
#include "CoreMinimal.h"

#define ARX_DEBUG_SNAPSHOT 1

#pragma optimize("",off)

class ARXRUNTIME_API ArxBasicSerializer
{
public:
    bool IsSaving()const{return bIsSaving;};
    bool IsLoading()const{return !bIsSaving;};
    
    virtual const FName& GetTypeName(){static const FName Name = "BasicSerializer"; return Name; }

    virtual ~ArxBasicSerializer(){};
    virtual ArxBasicSerializer& operator << (float& Value) = 0;
    virtual ArxBasicSerializer& operator << (double& Value) = 0;
    virtual ArxBasicSerializer& operator << ( uint32& Value) = 0;
    virtual ArxBasicSerializer& operator << ( int32& Value) = 0;
    virtual ArxBasicSerializer& operator << ( int64& Value) = 0;
    virtual ArxBasicSerializer& operator << ( uint64& Value) = 0;
    virtual ArxBasicSerializer& operator << ( FName& Value) = 0;
    virtual ArxBasicSerializer& operator << ( FString& Value) = 0;

    template<class Key, class Value>
    friend inline ArxBasicSerializer& operator << (ArxBasicSerializer& Ser, TPair<Key, Value>& Pair);

    template<class T>
    friend inline ArxBasicSerializer& operator << (ArxBasicSerializer& Ser, TArray<T>& Array);


protected:
    void SetIsSaving(bool Val) {bIsSaving = Val;}

private:
    bool bIsSaving = false;

};

class ARXRUNTIME_API ArxDataSerializer: public ArxBasicSerializer
{
public:
    static const FName TypeName;
    virtual const FName& GetTypeName() { return TypeName; }


    ArxDataSerializer(const TArray<uint8>& Data);
    ArxDataSerializer(TArray<uint8>& Data);

    void Serialize(void* Value, int Size);

    template<class T>
    ArxBasicSerializer& Serialize(T& Value) 
    {
        Serialize(&Value, sizeof(T));
        return *this;
    }
    ArxBasicSerializer& operator << (float& Value) override {checkf(false, TEXT("do not transmit floating point")); return *this; };
    ArxBasicSerializer& operator << (double& Value) override { checkf(false, TEXT("do not transmit floating point")); return *this; };
    ArxBasicSerializer& operator << (uint32& Value) override { return Serialize(Value); };
    ArxBasicSerializer& operator << (int32& Value) override { return Serialize(Value); };
    ArxBasicSerializer& operator << (int64& Value) override { return Serialize(Value); };
    ArxBasicSerializer& operator << (uint64& Value) override { return Serialize(Value); };

    virtual ArxBasicSerializer& operator << (FName& Value) override
    {
        if (IsSaving())
        {
            FString Val = Value.ToString();
            *this << Val;
        }
        else
        {
            FString Val;
            *this << Val;
            Value = FName(*Val);
        }
        return *this;
    }

    virtual ArxBasicSerializer& operator << (FString& Value) override
    {
        if (IsSaving())
        {
            auto Str = StringCast<ANSICHAR>(static_cast<const TCHAR*>(*(Value)));

            int Size = Str.Length();
            Serialize(&Size, sizeof(Size));
            Serialize((void*)Str.Get(), Size);
        }
        else
        {
            int Size ;
            Serialize(&Size, sizeof(Size));
            TArray<char> Data;
            Data.SetNumUninitialized(Size);
            Serialize((void*)Data.GetData(), Size);
            Data.AddDefaulted();
            Value = ANSI_TO_TCHAR(Data.GetData());
        }
        return *this;
    }
private:
    TArray<uint8>& Content;
    int ReadPos = 0;
};

class ARXRUNTIME_API ArxDebugSerializer: public ArxBasicSerializer
{
public:
    static const FName TypeName;
    virtual const FName& GetTypeName() { return TypeName; }

    ArxDebugSerializer(TArray<uint8>& Data);

    void Serialize(void* Value, int Size);

    template<class T>
    ArxBasicSerializer& Serialize(T& Value)
    {
        FString Val = LexToString(Value);
        *this << Val;
        return *this;
    }

    ArxBasicSerializer& operator << (float& Value) override { return Serialize(Value); };
    ArxBasicSerializer& operator << (double& Value) override { return Serialize(Value); };
    ArxBasicSerializer& operator << (uint32& Value) override { return Serialize(Value); };
    ArxBasicSerializer& operator << (int32& Value) override { return Serialize(Value); };
    ArxBasicSerializer& operator << (int64& Value) override { return Serialize(Value); };
    ArxBasicSerializer& operator << (uint64& Value) override { return Serialize(Value); };

    ArxBasicSerializer& operator << ( FName& Value) override
    {
        FString Val = Value.ToString();
        *this << Val;

        return *this;
    }

    ArxBasicSerializer& operator << (FString& Value) override
    {
        auto Str = StringCast<ANSICHAR>(static_cast<const TCHAR*>(* (Value)));
        Serialize((void*)Str.Get(), Str.Length());

        return *this;
    }
private:
    TArray<uint8>& Content;
};


template<class Key, class Value>
inline FString LexToString(const TPair<Key, Value>& Pair)
{
    return FString::Printf(TEXT("{%s, %s}"), *LexToString(Pair.Key), *LexToString(Pair.Value));
}

template<class Key, class Value>
inline ArxBasicSerializer& operator << (ArxBasicSerializer& Ser, TPair<Key, Value>& Pair)
{
#if ARX_DEBUG_SNAPSHOT
    if (Ser.GetTypeName() == ArxDebugSerializer::TypeName)
    {
        FString Ret = LexToString(Pair);
        Ser << Ret;
    }
    else
#endif
    {
        Ser << Pair.Key;
        Ser << Pair.Value;
    }

    return Ser;
}


template<class T>
inline ArxBasicSerializer& operator << (ArxBasicSerializer& Ser, TArray<T>& Array)
{
#if ARX_DEBUG_SNAPSHOT
    if (Ser.GetTypeName() == ArxDebugSerializer::TypeName)
    {
        FString Content;
        int Index = 0;
        for (auto& Item : Array)
        {
            Content += FString::Printf(TEXT("[%d] = %s,\n"),Index++, *LexToString(Item));
        }

        Content = FString::Printf(TEXT("{\n%s\n}\n"), *Content);
        Ser << Content;
    }
    else
#endif
	{
		if (Ser.IsSaving())
		{
			int Count = Array.Num();
			Ser << Count;
		}
		else
		{
			int Count;
            Ser << Count;
			Array.Empty(Count);
			Array.SetNum(Count);
		}

		for (auto& Item : Array)
		{
            Ser << Item;
		}
	}
    return Ser;
}

#pragma optimize("",on)



using ArxSerializer = ArxBasicSerializer;
using ArxReader = ArxDataSerializer;
using ArxWriter = ArxDataSerializer;


template< class T>
inline void MemberToString(ArxBasicSerializer& Ser, T& Val, const TCHAR* Name)
{
#if ARX_DEBUG_SNAPSHOT
    if (Ser.GetTypeName() == ArxDebugSerializer::TypeName)
    {
        FString Ret = FString::Printf(TEXT("%s = %s\n"), Name, *LexToString(Val));
        Ser << Ret;
    }
    else
#endif
    {
        Ser << Val;
    }
}

template<class T>
inline void ContainerToString(ArxBasicSerializer& Ser, T& Val, const TCHAR* Name)
{
#if ARX_DEBUG_SNAPSHOT
    if (Ser.GetTypeName() == ArxDebugSerializer::TypeName)
    {
    }
    else
#endif
    {
        if (Ser.IsSaving())
        {
            int Count = Val.Num();
            for (auto& Item : Val)
            {
                Ser << Item;
            }
        }
        else
        {

        }
    }
}

#define ARX_SERIALIZE_MEMBER_EX(Ser, Mem, Name) MemberToString(Ser, Mem, Name);
#define ARX_SERIALIZE_MEMBER(Ser, Mem) ARX_SERIALIZE_MEMBER_EX(Ser, Mem, TEXT(#Mem));
#define ARX_SERIALIZE_MEMBER_FAST(Mem) ARX_SERIALIZE_MEMBER(Serializer, Mem);