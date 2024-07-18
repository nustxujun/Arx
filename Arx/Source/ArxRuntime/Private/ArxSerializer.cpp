#include "ArxSerializer.h"

const FName ArxDataSerializer::TypeName = "DataSerializer";
const FName ArxDebugSerializer::TypeName = "DebugSerializer";


ArxDataSerializer::ArxDataSerializer(const TArray<uint8>& Data)
:Content(const_cast<TArray<uint8>&>(Data))

{
	SetIsSaving(false);
}

ArxDataSerializer::ArxDataSerializer(TArray<uint8>& Data)
:Content(Data)
{
	SetIsSaving(true);
}



void ArxDataSerializer::Serialize(void* Value, int Size)
{
	if (IsSaving())
	{
		auto Begin = Content.Num();
		Content.AddUninitialized(Size);
		
		FMemory::Memcpy(&Content[Begin], Value, Size );
	}
	else
	{
		check(ReadPos + Size <= Content.Num())
		FMemory::Memcpy(Value, &Content[ReadPos], Size);
		ReadPos += Size;
	}
}

bool ArxDataSerializer::AtEnd()
{
	if (IsSaving())
	{
		return false;
	}
	else
	{
		return ReadPos >= Content.Num();
	}
}


ArxDebugSerializer::ArxDebugSerializer(TArray<uint8>& Data) :Content(Data)
{
	SetIsSaving(true);
}

void ArxDebugSerializer::Serialize(void* Value, int Size)
{
	auto Begin = Content.Num();
	Content.AddUninitialized(Size);

	FMemory::Memcpy(&Content[Begin], Value, Size);
}

