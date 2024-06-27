#include "ArxSerializer.h"
#pragma optimize("",off)

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
		FMemory::Memcpy(Value, &Content[ReadPos], Size);
		ReadPos += Size;
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

#pragma optimize("",on)
