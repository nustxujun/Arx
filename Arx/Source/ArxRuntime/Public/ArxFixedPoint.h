#pragma once

#include "CoreMinimal.h"
#include <stdint.h>
// #include <ios>
// #include <iostream> 
// #include <string>

namespace Arx
{
template<unsigned int FractionBits, bool bOverflow = true, bool bSaturating = false>
class FixedPoint64
{
public:
    static constexpr unsigned int TotalBits = 64;
    static_assert(FractionBits < TotalBits - 1, "fraction can be greater than 62");

    using fixed_raw = int64_t;
    using internal_type = uint64_t;
    static constexpr fixed_raw FRACTION = fixed_raw(1) << FractionBits;

    static constexpr internal_type FRACTION_MASK = (~internal_type(0)) >> (TotalBits - FractionBits);
    static constexpr internal_type SIGN_MASK = 0x8000'0000'0000'0000;
    static constexpr fixed_raw MAXIMUM = 0x7FFF'FFFF'FFFF'FFFF;
    static constexpr fixed_raw MINIMUM = -MAXIMUM;
public:
    constexpr FORCEINLINE FixedPoint64() noexcept = default;


    constexpr FORCEINLINE FixedPoint64(const FixedPoint64& val) noexcept
        :value(val.value)
    {
    }

    template<unsigned int F>
    constexpr FORCEINLINE FixedPoint64(FixedPoint64<F> val) noexcept
        :value(from_fixed<F>(val).raw_value())
    {
    }

    template <typename T, typename std::enable_if<std::is_integral<T>::value>::type* = nullptr>
    constexpr FORCEINLINE  FixedPoint64(T val) noexcept
        : value(static_cast<fixed_raw>(val)* FRACTION)
    {
    }

    template <typename T, typename std::enable_if<std::is_floating_point<T>::value>::type* = nullptr>
    constexpr FORCEINLINE  FixedPoint64(T val) noexcept
        : value(static_cast<fixed_raw>(val* FRACTION))
    {
    }

    template <typename T, typename std::enable_if<std::is_floating_point<T>::value>::type* = nullptr>
    constexpr FORCEINLINE explicit operator T() const noexcept
    {
        return static_cast<T>(value) / static_cast<T>(FRACTION);
    }

    // Explicit conversion to an integral type
    template <typename T, typename std::enable_if<std::is_integral<T>::value>::type* = nullptr>
    constexpr FORCEINLINE explicit operator T() const noexcept
    {
        return static_cast<T>(value / FRACTION);
    }

    constexpr FORCEINLINE fixed_raw raw_value()const noexcept
    {
        return value;
    }

    static constexpr FORCEINLINE FixedPoint64 from_raw(fixed_raw val) noexcept
    {
        FixedPoint64 ret{};
        ret.value = val;
        return ret;
    }

    constexpr FORCEINLINE FixedPoint64 operator-() const noexcept
    {
        return from_raw(-value);
    }

    constexpr FORCEINLINE FixedPoint64& operator+=(FixedPoint64 val) noexcept
    {
        if (bOverflow)
        {
            internal_type v1 = value, v2 = val.value;
            internal_type sum = v1 + v2;
            value = static_cast<fixed_raw>(sum);

            if (!((v1 ^ v2) & SIGN_MASK) && ((v1 ^ sum) & SIGN_MASK))
            {
                checkf(bSaturating, TEXT("overflow!"));
                value = (val.value >= 0) ? MAXIMUM : MINIMUM;
            }
        }
        else
        {
            value += val.value;
        }
        return *this;
    }

    constexpr FORCEINLINE FixedPoint64& operator-=(FixedPoint64 val) noexcept
    {
        return *this += (-val);
    }

    friend constexpr FORCEINLINE FixedPoint64 operator+ (FixedPoint64 v1, FixedPoint64 v2) noexcept
    {
        return v1 += v2;
    }

    friend constexpr FORCEINLINE FixedPoint64 operator- (FixedPoint64 v1, FixedPoint64 v2) noexcept
    {
        return v1 -= v2;
    }

    constexpr FORCEINLINE FixedPoint64& operator*= (FixedPoint64 val) noexcept
    {
        fixed_raw A = value >> 32, C = val.value >> 32;
        internal_type B = value & 0xFFFFFFFF, D = val.value & 0xFFFFFFFF;

        auto AC = A * C;
        fixed_raw AD_CB = A * D + C * B;
        auto BD = B * D;

        fixed_raw product_hi = AC + (AD_CB >> 32);
        internal_type product_lo = BD + (internal_type(AD_CB) << 32);
        if (product_lo < BD)
            product_hi++;

        if (bOverflow)
        {
            if ((product_hi >> 63) != (product_hi >> (FractionBits - 1)))
            {
                checkf(bSaturating, TEXT("overflow!"));
                if (((value ^ val.value) & SIGN_MASK) == 0)
                    value = MAXIMUM;
                else
                    value = MINIMUM;

                return *this;
            }
        }


        value = (product_hi << (64 - FractionBits)) | (product_lo >> FractionBits);
        return *this;
    }

    friend constexpr FORCEINLINE FixedPoint64 operator* (FixedPoint64 v1, FixedPoint64 v2) noexcept
    {
        return v1 *= v2;
    }

    constexpr FORCEINLINE FixedPoint64& operator/= (FixedPoint64 val) noexcept
    {
        if (val.value == 0)
        {
            if (value > 0)
                value = MAXIMUM;
            else
                value = MINIMUM;
            return *this;
        }

        internal_type remainder = (value >= 0) ? value : (-value);
        internal_type divider = (val.value >= 0) ? val.value : (-val.value);
        internal_type quotient = 0;
        int bit_pos = FractionBits + 1;


        // If the divider is divisible by 2^n, take advantage of it.
        while (!(divider & 0xF) && bit_pos >= 4)
        {
            divider >>= 4;
            bit_pos -= 4;
        }

        while (remainder && bit_pos >= 0)
        {
            // Shift remainder as much as we can without overflowing
            auto shift = clz(remainder);
            if (shift > bit_pos) shift = bit_pos;
            remainder <<= shift;
            bit_pos -= shift;

            internal_type div = remainder / divider;
            remainder = remainder % divider;
            quotient += div << bit_pos;

            if (bOverflow)
            {
                if (div & ~((~fixed_raw(0)) >> bit_pos))
                {
                    checkf(bSaturating, TEXT("overflow!"));

                    if (((value ^ val.value) & SIGN_MASK) == 0)
                        value = MAXIMUM;
                    else
                        value = MINIMUM;

                    return *this;
                }
            }

            remainder <<= 1;
            bit_pos--;
        }


        fixed_raw result = quotient >> 1;

        // Figure out the sign of the result
        if ((value ^ val.value) & SIGN_MASK)
        {
            if (bOverflow)
            {
                if (result == SIGN_MASK)
                {
                    checkf(bSaturating, TEXT("overflow!"));
                    value = MINIMUM;
                    return *this;
                }
            }
            result = -result;
        }

        value = result;
        return *this;
    }

    friend constexpr FORCEINLINE FixedPoint64 operator/ (FixedPoint64 v1, FixedPoint64 v2) noexcept
    {
        return v1 /= v2;
    }

    friend constexpr FORCEINLINE bool operator< (FixedPoint64 v1, FixedPoint64 v2) noexcept
    {
        return v1.raw_value() < v2.raw_value();
    }

    friend constexpr FORCEINLINE bool operator<= (FixedPoint64 v1, FixedPoint64 v2) noexcept
    {
        return v1.raw_value() <= v2.raw_value();
    }

    friend constexpr FORCEINLINE bool operator== (FixedPoint64 v1, FixedPoint64 v2) noexcept
    {
        return v1.raw_value() == v2.raw_value();
    }

    friend constexpr FORCEINLINE bool operator!= (FixedPoint64 v1, FixedPoint64 v2) noexcept
    {
        return v1.raw_value() != v2.raw_value();
    }

    friend constexpr FORCEINLINE bool operator>= (FixedPoint64 v1, FixedPoint64 v2) noexcept
    {
        return v1.raw_value() >= v2.raw_value();
    }

    friend constexpr FORCEINLINE bool operator> (FixedPoint64 v1, FixedPoint64 v2) noexcept
    {
        return v1.raw_value() > v2.raw_value();
    }

    static constexpr FORCEINLINE long clz(internal_type value) noexcept
    {
#if defined(_MSC_VER) 
        if (value == 0) return 64;
        unsigned long index;
        _BitScanReverse64(&index, value);
        return 63 - index;
#elif (defined(__GNUC__) || defined(__clang__)) 
        return __builtin_clzll(value);
#else
        uint8_t result = 0;
        if (value == 0) return 64;
        while (!(value & 0xF000000000000000)) { result += 4; value <<= 4; }
        while (!(value & 0x8000000000000000)) { result += 1; value <<= 1; }
        return result;
#endif
    }


    static constexpr FORCEINLINE FixedPoint64 e() { return FixedPoint64(FixedPoint64<61>::from_raw(6267931151224907085ll)); }
    static constexpr FORCEINLINE FixedPoint64 pi() { return FixedPoint64(FixedPoint64<61>::from_raw(7244019458077122842ll)); }
    static constexpr FORCEINLINE FixedPoint64 half_pi() { return FixedPoint64(FixedPoint64<62>::from_raw(7244019458077122842ll)); }
    static constexpr FORCEINLINE FixedPoint64 two_pi() { return FixedPoint64(FixedPoint64<60>::from_raw(7244019458077122842ll)); }
public:

    template<unsigned int F, typename std::enable_if<(FractionBits > F)>::type* = nullptr>
        static constexpr FORCEINLINE FixedPoint64<F> from_fixed(FixedPoint64<F> val)noexcept
    {
        return FixedPoint64<F>::from_raw(val.raw_value() * (fixed_raw(1) << (FractionBits - F)));
    }

    template<unsigned int F, typename std::enable_if<(FractionBits <= F)>::type* = nullptr>
    static constexpr FORCEINLINE FixedPoint64<F> from_fixed(FixedPoint64<F> val)noexcept
    {
        return FixedPoint64<F>::from_raw(val.raw_value() / (fixed_raw(1) << (F - FractionBits)));
    }





private:
    fixed_raw value;
};

template<unsigned int F>
constexpr FORCEINLINE FixedPoint64<F> ceil(FixedPoint64<F> v) noexcept
{
    constexpr auto FRAC = FixedPoint64<F>::FRACTION;
    auto value = v.raw_value();
    if (value > 0) value += FRAC - 1;
    return FixedPoint64<F>::from_raw(value / FRAC * FRAC);
}

template<unsigned int F>
constexpr FORCEINLINE FixedPoint64<F> floor(FixedPoint64<F> v) noexcept
{
    constexpr auto FRAC = FixedPoint64<F>::FRACTION;
    auto value = v.raw_value();
    if (value < 0) value -= FRAC - 1;
    return FixedPoint64<F>::from_raw(value / FRAC * FRAC);
}

template<unsigned int F>
constexpr FORCEINLINE FixedPoint64<F> round(FixedPoint64<F> v) noexcept

{
    constexpr auto FRAC = FixedPoint64<F>::FRACTION;
    auto value = v.raw_value() / (FRAC / 2);
    return FixedPoint64<F>::from_raw(((value / 2) + (value % 2)) * FRAC);
}


template<unsigned int F>
constexpr FORCEINLINE FixedPoint64<F> abs(FixedPoint64<F> v) noexcept
{
    return (v >= FixedPoint64<F>{0}) ? v : -v;
}

template<unsigned int F>
constexpr FORCEINLINE FixedPoint64<F> fmod(FixedPoint64<F> a, FixedPoint64<F> b) noexcept
{
    check(b.raw_value() != 0);
    return FixedPoint64<F>::from_raw(a.raw_value() % b.raw_value());
}


template <unsigned int F, class T, typename std::enable_if<std::is_integral<T>::value>::type* = nullptr>
constexpr inline FixedPoint64<F> pow(FixedPoint64<F> base, T exp) noexcept
{
    using Fixed = FixedPoint64<F>;

    if (base == Fixed(0)) {
        check(exp > 0);
        return Fixed(0);
    }

    Fixed result{ 1 };
    if (exp < 0)
    {
        for (Fixed intermediate = base; exp != 0; exp /= 2)
        {
            if ((exp % 2) != 0)
            {
                result /= intermediate;
            }
            if (exp != 1)
                intermediate *= intermediate;
        }
    }
    else
    {
        for (Fixed intermediate = base; exp != 0; exp /= 2)
        {
            if ((exp % 2) != 0)
            {
                result *= intermediate;
            }
            if (exp != 1)
                intermediate *= intermediate;
        }
    }
    return result;
}

template <unsigned int F>
constexpr inline FixedPoint64<F> pow(FixedPoint64<F> base, FixedPoint64<F> exp) noexcept
{
    using Fixed = FixedPoint64<F>;

    if (base == Fixed(0)) {
        check(exp > Fixed(0));
        return Fixed(0);
    }

    if (exp < Fixed(0))
    {
        return 1 / pow(base, -exp);
    }

    constexpr auto FRAC = Fixed::FRACTION;
    if (exp.raw_value() % FRAC == 0)
    {
        return pow(base, exp.raw_value() / FRAC);
    }

    check(base > Fixed(0));
    return exp2(log2(base) * exp);
}


template <unsigned int F>
constexpr inline FixedPoint64<F> exp(FixedPoint64<F> x) noexcept
{
    using Fixed = FixedPoint64<F>;
    if (x < Fixed(0)) {
        return 1 / exp(-x);
    }
    constexpr auto FRAC = Fixed::FRACTION;
    auto x_int = x.raw_value() / FRAC;
    x -= x_int;
    check(x >= Fixed(0) && x < Fixed(1));

    constexpr Fixed fA = Fixed(1.3903728105644451e-2); // 
    constexpr Fixed fB = Fixed(3.4800571158543038e-2); // 
    constexpr Fixed fC = Fixed(1.7040197373796334e-1); // 
    constexpr Fixed fD = Fixed(4.9909609871464493e-1); // 
    constexpr Fixed fE = Fixed(1.0000794567422495); // 
    constexpr Fixed fF = Fixed(9.9999887043019773e-1); // 

    return  pow(Fixed::e(), x_int) * (((((fA * x + fB) * x + fC) * x + fD) * x + fE) * x + fF);
}

template < unsigned int F>
constexpr inline FixedPoint64<F> exp2(FixedPoint64<F> x) noexcept
{
    using Fixed = FixedPoint64<F>;
    if (x < Fixed(0)) {
        return 1 / exp2(-x);
    }

    constexpr auto FRAC = Fixed::FRACTION;
    const auto x_int = x.raw_value() / FRAC;
    x -= x_int;
    check(x >= Fixed(0) && x < Fixed(1));

    constexpr auto fA = Fixed(1.8964611454333148e-3);
    constexpr auto fB = Fixed(8.9428289841091295e-3);
    constexpr auto fC = Fixed(5.5866246304520701e-2);
    constexpr auto fD = Fixed(2.4013971109076949e-1);
    constexpr auto fE = Fixed(6.9315475247516736e-1);
    constexpr auto fF = Fixed(9.9999989311082668e-1);
    return Fixed(1 << x_int) * (((((fA * x + fB) * x + fC) * x + fD) * x + fE) * x + fF);
}

template < unsigned int F>
constexpr inline FixedPoint64<F> log2(FixedPoint64<F> x) noexcept
{
    using Fixed = FixedPoint64<F>;
    check(x > Fixed(0));

    // Normalize input to the [1:2] domain
    auto value = x.raw_value();
    const long highest = sizeof(decltype(value)) * 8 - Fixed::clz(value) - 1;
    if (highest >= F) {
        value >>= (highest - F);
    }
    else {
        value <<= (F - highest);
    }
    x = Fixed::from_raw(value);
    check(x >= Fixed(1) && x < Fixed(2));

    constexpr auto fA = Fixed(4.4873610194131727e-2);
    constexpr auto fB = Fixed(-4.1656368651734915e-1);
    constexpr auto fC = Fixed(1.6311487636297217);
    constexpr auto fD = Fixed(-3.5507929249026341);
    constexpr auto fE = Fixed(5.0917108110420042);
    constexpr auto fF = Fixed(-2.8003640347009253);
    return Fixed(highest - F) + (((((fA * x + fB) * x + fC) * x + fD) * x + fE) * x + fF);
}

template <unsigned int F>
constexpr FixedPoint64<F> log(FixedPoint64<F> x) noexcept
{
    using Fixed = FixedPoint64<F>;
    return log2(x) / log2(Fixed::e());
}

template <typename B, typename I, unsigned int F, bool R>
constexpr FixedPoint64<F> log10(FixedPoint64<F> x) noexcept
{
    using Fixed = FixedPoint64<F>;
    return log2(x) / log2(Fixed(10));
}


template<unsigned int F>
constexpr FORCEINLINE FixedPoint64<F> sqrt(FixedPoint64<F> v)
{
    /*
        from https://en.wikipedia.org/wiki/Methods_of_computing_square_roots#Binary_numeral_system_.28base_2.29
    */
    check(("sqrt input should be non-negative", v >= 0));

    using Fixed = FixedPoint64<F>;

    uint64_t n = v.raw_value();
    n <<= (F & 1);

    uint64_t x = n;
    uint64_t c = 0;

    uint64_t d = uint64_t(1) << ((64 - Fixed::clz(n) - 1) & ~(1));

    while (d != 0)
    {
        auto a = c + d;
        if (x >= a)
        {
            x -= a;
            c = (c >> 1) + d;
        }
        else
        {
            c >>= 1;
        }
        d >>= 2;
    }

    return Fixed::from_raw(int64_t(c << (F / 2)));
}

template <unsigned int F>
constexpr FORCEINLINE FixedPoint64<F> copysign(FixedPoint64<F> x, FixedPoint64<F> y) noexcept
{
    x = abs(x);
    return (y >= FixedPoint64<F>{0}) ? x : -x;
}

template <unsigned int F>
constexpr inline FixedPoint64<F> sin(FixedPoint64<F> x) noexcept
{
    using Fixed = FixedPoint64<F>;

    x = fmod(x, Fixed::two_pi());
    x = x / Fixed::half_pi();

    if (x < Fixed(0)) {
        x += Fixed(4);
    }

    int sign = +1;
    if (x > Fixed(2)) {
        sign = -1;
        x -= Fixed(2);
    }

    if (x > Fixed(1)) {
        x = Fixed(2) - x;
    }

    const Fixed x2 = x * x;
    return sign * x * (Fixed::pi() - x2 * (Fixed::two_pi() - 5 - x2 * (Fixed::pi() - 3))) / 2;
}

template <unsigned int F>
constexpr FORCEINLINE FixedPoint64<F> cos(FixedPoint64<F> x) noexcept
{
    using Fixed = FixedPoint64<F>;
    if (x > Fixed(0)) {
        return sin(x - (Fixed::two_pi() - Fixed::half_pi()));
    }
    else {
        return sin(Fixed::half_pi() + x);
    }
}

template <unsigned int F>
constexpr inline FixedPoint64<F> tan(FixedPoint64<F> x) noexcept
{
    auto cx = cos(x);

    check(abs(cx).raw_value() > 1);

    return sin(x) / cx;
}


namespace internal
{
    template <unsigned int F>
    constexpr inline FixedPoint64<F> atan_sanitized(FixedPoint64<F> x)
    {
        using Fixed = FixedPoint64<F>;
        check(x >= Fixed(0) && x <= Fixed(1));

        constexpr auto fA = Fixed(0.0776509570923569);
        constexpr auto fB = Fixed(-0.287434475393028);
        constexpr auto fC = Fixed(0.995181681698119); //    (PI/4 - A - B)

        const auto xx = x * x;
        return ((fA * xx + fB) * xx + fC) * x;
    };
    template <unsigned int F>
    constexpr inline FixedPoint64<F> atan_div(FixedPoint64<F> y, FixedPoint64<F> x) noexcept
    {
        using Fixed = FixedPoint64<F>;
        check(x != Fixed(0));

        if (y < Fixed(0)) {
            if (x < Fixed(0)) {
                return atan_div(-y, -x);
            }
            return -atan_div(-y, x);
        }
        if (x < Fixed(0)) {
            return -atan_div(y, -x);
        }
        check(y >= Fixed(0));
        check(x > Fixed(0));

        if (y > x) {
            return Fixed::half_pi() - internal::atan_sanitized(x / y);
        }
        return internal::atan_sanitized(y / x);
    }
}

template <unsigned int F>
constexpr inline FixedPoint64<F> atan(FixedPoint64<F> x) noexcept
{
    using Fixed = FixedPoint64<F>;

    if (x < Fixed(0))
    {
        return -atan(-x);
    }

    if (x > Fixed(1))
    {
        return Fixed::half_pi() - internal::atan_sanitized(Fixed(1) / x);
    }

    return internal::atan_sanitized(x);
}

template <unsigned int F>
constexpr inline FixedPoint64<F> atan2(FixedPoint64<F> y, FixedPoint64<F> x) noexcept
{
    using Fixed = FixedPoint64<F>;
    if (x == Fixed(0))
    {
        check(y != Fixed(0));
        return (y > Fixed(0)) ? Fixed::half_pi() : -Fixed::half_pi();
    }

    auto ret = internal::atan_div(y, x);

    if (x < Fixed(0))
    {
        return (y >= Fixed(0)) ? ret + Fixed::pi() : ret - Fixed::pi();
    }
    return ret;
}


template <unsigned int F>
constexpr inline FixedPoint64<F> asin(FixedPoint64<F> x) noexcept
{
    using Fixed = FixedPoint64<F>;
    check(x >= Fixed(-1) && x <= Fixed(+1));

    const auto yy = Fixed(1) - x * x;
    if (yy == Fixed(0))
    {
        return copysign(Fixed::half_pi(), x);
    }
    return internal::atan_div(x, sqrt(yy));
}

template <unsigned int F>
constexpr inline FixedPoint64<F> acos(FixedPoint64<F> x) noexcept
{
    using Fixed = FixedPoint64<F>;
    check(x >= Fixed(-1) && x <= Fixed(+1));

    if (x == Fixed(-1))
    {
        return Fixed::pi();
    }
    const auto yy = Fixed(1) - x * x;
    return Fixed(2) * internal::atan_div(sqrt(yy), Fixed(1) + x);
}

}


template<unsigned int F, bool bOverflow , bool bSaturating >
inline FString LexToString(const Arx::FixedPoint64<F,  bOverflow, bSaturating>& Value)
{
    return FString::Printf(TEXT("%.16lf(%x)"), (double)Value, Value.raw_value());
}