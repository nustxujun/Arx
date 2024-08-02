#pragma once 
#include "ArxCommon.h"
#include "Rp3dCommon.h"
#include "Rp3dUtils.h"

#include "reactphysics3d/fixed64.hpp"

using ArxFixed64 = f64::fixed64<32>;
using ArxTimePoint = ArxFixed64;
using ArxTimeDuration = ArxFixed64;

template<unsigned int F>
FORCEINLINE uint32 GetTypeHash(f64::fixed64<F> Value)
{
    return GetTypeHash(Value.raw_value());
}

template<unsigned int F>
FORCEINLINE ArxSerializer& operator << (ArxSerializer& Ser, f64::fixed64<F>& Value)
{
	return Ser << (*(int64*) & Value);
}

template<unsigned int F>
inline FString LexToString(const f64::fixed64<F>& Value)
{
	return FString::Printf(TEXT("%.16lf(%x)"), (double)Value, Value.raw_value());
}

FORCEINLINE uint32 GetTypeHash(const Rp3dVector3& Vector)
{
	return FCrc::MemCrc32(&Vector, sizeof(Vector));
}

#ifdef RP3D_USE_FIXED
inline FString LexToString(const Rp3dVector3& Value)
{
	return FString::Printf(TEXT("Vec3{%s(%x), %s(%x), %s(%x)}"), *LexToString(RP3D_TO_UE(Value.x)), Value.x.raw_value(), *LexToString(RP3D_TO_UE(Value.y)), Value.y.raw_value(), *LexToString(RP3D_TO_UE(Value.z)), Value.z.raw_value());
}

inline FString LexToString(const Rp3dQuat& Value)
{
	return FString::Printf(TEXT("Quat{%s(%x), %s(%x), %s(%x), %s(%x)}"), *LexToString(Value.x), Value.x.raw_value(), *LexToString(Value.y), Value.y.raw_value(), *LexToString(Value.z), Value.z.raw_value(), *LexToString(Value.w), Value.w.raw_value());
}
#else
inline FString LexToString(const Rp3dVector3& Value)
{
	return FString::Printf(TEXT("Vec3{%s, %s, %s}"), *LexToString(RP3D_TO_UE(Value.x)), *LexToString(RP3D_TO_UE(Value.y)), *LexToString(RP3D_TO_UE(Value.z)));
}

inline FString LexToString(const Rp3dQuat& Value)
{
	return FString::Printf(TEXT("Quat{%s, %s, %s, %s}"), *LexToString(Value.x), *LexToString(Value.y), *LexToString(Value.z), *LexToString(Value.w));
}
#endif

FORCEINLINE ArxSerializer& operator << (ArxSerializer& Ser, Rp3dVector3& Vector)
{
	if (Ser.GetTypeName() == ArxDebugSerializer::TypeName)
	{
		FString Str = LexToString(Vector);
		Ser << Str;
	}
	else
	{
		Ser << Vector.x;
		Ser << Vector.y;
		Ser << Vector.z;
	}
	return Ser;
}


FORCEINLINE uint32 GetTypeHash(const Rp3dQuat& Quat)
{
	return FCrc::MemCrc32(&Quat, sizeof(Quat));
}


FORCEINLINE ArxSerializer& operator << (ArxSerializer& Ser, Rp3dQuat& Quat)
{
	if (Ser.GetTypeName() == ArxDebugSerializer::TypeName)
	{
		FString Str = LexToString(Quat);
		Ser << Str;
	}
	else
	{
		Ser << Quat.x;
		Ser << Quat.y;
		Ser << Quat.z;
		Ser << Quat.w;
	}
	return Ser;
}


FORCEINLINE uint32 GetTypeHash(const Rp3dTransform& Trans)
{
	return FCrc::MemCrc32(&Trans, sizeof(Trans));
}

inline FString LexToString(const Rp3dTransform& Value)
{
	return FString::Printf(TEXT("Transfrom{%s, %s}"), *LexToString(Value.getPosition()), *LexToString(Value.getOrientation()));
}

inline ArxSerializer& operator << (ArxSerializer& Ser, Rp3dTransform& Trans)
{
	if (Ser.GetTypeName() == ArxDebugSerializer::TypeName)
	{
		FString Str = LexToString(Trans);
		Ser << Str;
	}
	else
	{
		if (Ser.IsSaving())
		{
			Ser << const_cast<Rp3dVector3&>(Trans.getPosition());
			Ser << const_cast<Rp3dQuat&>(Trans.getOrientation());
		}
		else
		{
			Rp3dVector3 Pos;
			Rp3dQuat Orin;
			Ser << Pos;
			Ser << Orin;
			Trans.setPosition(Pos);
			Trans.setOrientation(Orin);
		}
	}
	return Ser;
}


class ArxConstants
{
public:
    static constexpr ArxTimeDuration TimeStep = 1.0 / 15.0;
	static constexpr int NumPhysicsStep = 4;
	static constexpr int VerificationCycle = 1; // frame
};