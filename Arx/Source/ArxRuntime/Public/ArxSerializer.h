#pragma once 
#include "CoreMinimal.h"

#define ARX_DEBUG_SNAPSHOT 1


class ARXRUNTIME_API ArxBasicSerializer
{
public:
    bool IsSaving()const{return bIsSaving;};
    bool IsLoading()const{return !bIsSaving;};
    virtual bool AtEnd() = 0;

    virtual const FName& GetTypeName(){static const FName Name = "BasicSerializer"; return Name; }

    virtual ~ArxBasicSerializer(){};

    virtual ArxBasicSerializer& operator << (bool& Value) = 0;
    virtual ArxBasicSerializer& operator << (int8& Value) = 0;
    virtual ArxBasicSerializer& operator << (uint8& Value) = 0;
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

    template<class Key, class Value>
    friend inline ArxBasicSerializer& operator << (ArxBasicSerializer& Ser, TSortedMap<Key,Value>& Map);

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
    ArxBasicSerializer& operator << (float& Value) override {
#ifdef RP3D_USE_FIXED
        checkf(false, TEXT("do not transmit floating point")); 
#else
        Serialize(Value);
#endif
        return *this; 
        };
    ArxBasicSerializer& operator << (double& Value) override { 
#ifdef RP3D_USE_FIXED
        checkf(false, TEXT("do not transmit floating point")); 
#else
        Serialize(Value);
#endif       
        return *this; };
    ArxBasicSerializer& operator << (bool& Value) override { return Serialize(Value); };
    ArxBasicSerializer& operator << (int8& Value) override { return Serialize(Value); };
    ArxBasicSerializer& operator << (uint8& Value) override { return Serialize(Value); };
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
            if (Size != 0)
                Serialize((void*)Str.Get(), Size);
        }
        else
        {
            int Size ;
            Serialize(&Size, sizeof(Size));
            TArray<char> Data;
            if (Size != 0)
            {
                Data.SetNumUninitialized(Size);
                Serialize((void*)Data.GetData(), Size);
            }
            Data.AddDefaulted();
            Value = ANSI_TO_TCHAR(Data.GetData());
        }
        return *this;
    }

    bool AtEnd()override;
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

    ArxBasicSerializer& operator << (bool& Value) override { return Serialize(Value); };
    ArxBasicSerializer& operator << (int8& Value) override { return Serialize(Value); };
    ArxBasicSerializer& operator << (uint8& Value) override { return Serialize(Value); };
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
        if (!Value.IsEmpty())
            Serialize((void*)GetData(Value), Value.Len() * sizeof(TCHAR));

        return *this;
    }
    bool AtEnd() override {return false;}
private:
    TArray<uint8>& Content;
};


template<class Key, class Value>
inline FString LexToString(const TPair<Key, Value>& Pair)
{
    return FString::Printf(TEXT("{%s, %s}"), *LexToString(Pair.Key), *LexToString(Pair.Value));
}

template<class T>
inline FString LexToString(const TArray<T>& Array)
{
    FString Content;
    for (auto& Item : Array)
    {
        Content += FString::Printf(TEXT("\t%s,\n"), *LexToString(Item));
    }

    return FString::Printf(TEXT("{\n%s\n}\n"), *Content);
}

template<class Key, class Value>
FString LexToString(const TSortedMap<Key, Value>& Map)
{
    FString Content = TEXT("{\n");
    for (auto& Item : Map)
    {
        Content += LexToString(Item) + TEXT(",\n");
    }

    return Content + TEXT("}\n");
}

template<class Key, class Value>
inline ArxBasicSerializer& operator << (ArxBasicSerializer& Ser, TPair<Key, Value>& Pair)
{
    Ser << Pair.Key;
    Ser << Pair.Value;

    return Ser;
}


template<class T>
inline ArxBasicSerializer& operator << (ArxBasicSerializer& Ser, TArray<T>& Array)
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
    return Ser;
}

template<class Key, class Value>
inline ArxBasicSerializer& operator << (ArxBasicSerializer& Ser, TSortedMap<Key,Value>& Map)
{
#if ARX_DEBUG_SNAPSHOT 
    if (Ser.GetTypeName() == ArxDebugSerializer::TypeName)
    {
        FString Str = TEXT(" {\n");
        for (auto& Item : Map)
        {
            Str += FString::Printf(TEXT("%s = %s\n"), *LexToString(Item.Key), *LexToString(Item.Value));
        }
        Str += TEXT("}\n");

        Ser << Str;
    }
    else
#endif
    if (Ser.IsSaving())
    {
        int Count = Map.Num();
        Ser << Count;

        for (auto& Item : Map)
        {
            Ser << Item;
        }
    }
    else
    {
        Map.Reset();
        int Count;
        Ser << Count;
        for (int i = 0; i < Count; ++i)
        {
            Key k;
            Value v;
            Ser << k << v;
            Map.Add(MoveTemp(k), MoveTemp(v));
        }
    }

    return Ser;
}



class ArxReader : public ArxDataSerializer
{
public: 
    ArxReader(const TArray<uint8>& Data): ArxDataSerializer(Data){};
};

class ArxWriter : public ArxDataSerializer
{
public:
    ArxWriter(TArray<uint8>& Data) : ArxDataSerializer(Data) {};
};

using ArxSerializer = ArxBasicSerializer;




template< class T>
inline void SerializeMember(ArxBasicSerializer& Ser, T& Val, const TCHAR* Name)
{
#if ARX_DEBUG_SNAPSHOT
    if (Ser.GetTypeName() == ArxDebugSerializer::TypeName)
    {
        FString Ret = FString::Printf(TEXT("\n%s = "), Name);
        Ser << Ret;
    }
#endif
    Ser << Val;
}


#define ARX_SERIALIZE_MEMBER_EX(Ser, Mem, Name) SerializeMember(Ser, Mem, Name);
#define ARX_SERIALIZE_MEMBER(Ser, Mem) ARX_SERIALIZE_MEMBER_EX(Ser, Mem, TEXT(#Mem));
#define ARX_SERIALIZE_MEMBER_FAST(Mem) ARX_SERIALIZE_MEMBER(Serializer, Mem);