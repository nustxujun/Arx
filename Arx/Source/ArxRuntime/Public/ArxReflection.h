#pragma once 

namespace Arx
{
    template <size_t, class, template <size_t, class> class, class = void>
    struct FieldIndexImpl {
        static constexpr size_t Index = 0;
    };

    template <size_t I, class M, template <size_t, class> class T>
    struct FieldIndexImpl<I, M, T, std::void_t< typename T<I,M>::Exist >>
    {
        static constexpr size_t Index = 1 + FieldIndexImpl<I + 1, M, T>::Index;
    };

    template <class M, template <size_t, class> class T>
    struct FieldIndex {
        static constexpr size_t Index = FieldIndexImpl<0, M, T>::Index;
    };

    template <class T>
    constexpr size_t GetNumFields()
    {
        return FieldIndex<struct FieldTag, T::template FieldReflection>::Index;
    } 

    template <class T, size_t I>
    constexpr auto GetFieldName()
    {
        return T::template FieldReflection<I, struct FieldTag>::Name;
    } 

    template <class T, size_t I>
    using FieldType_T = typename T::template FieldReflection< I, struct FieldTag>::Type; 

    template <class T, size_t I>
    auto& GetField(T& Val) noexcept 
    {
        constexpr auto Ptr = T::template FieldReflection<I, struct FieldTag>::template Offset<T>;
        return Val.*Ptr;
    }

    template <class T, size_t I>
    const auto& GetField(const T& Val) noexcept 
    {
        constexpr auto Ptr = T::template FieldReflection< I, struct FieldTag>::template Offset<T>;
        return Val.*Ptr;
    }


    template<size_t I, class T, class F>
    void VisitSingleField(T& val, const F& f)
    {
        f(GetField< T, I>(val));
    }

    template<class F, class T,  size_t ... Indices>
    void VisitFields(T& val, const F& f, std::index_sequence<Indices...>)
    {
        constexpr auto N = sizeof...(Indices);
        int a[] = { (VisitSingleField<Indices, T,F>(val,f), 0) ... };
        (void)a;
        (void)N;
    }

    template<class T,class F>
    void Visit(T& val, const F& f)
    {
        VisitFields<F,T>(val, f,std::make_index_sequence<GetNumFields<T>()>());
    }

}


#define REFLECT_BEGIN()

#define REFLECT_FIELD(TYPE, NAME) \
TYPE NAME;\
template <size_t, class> struct FieldReflection;\
template <class T> struct FieldReflection<Arx::FieldIndex<struct FieldTag_##NAME,FieldReflection>::Index, T> {\
    using Type = TYPE;\
    using Exist = void;\
    constexpr static char Name[] = #NAME;\
    template <class U>\
    static constexpr Type U::*Offset = &U::NAME;\
};  

#define REFLECT_END()\
struct Reflection{\
    static constexpr size_t GetNumFields() noexcept{\
        return Arx::FieldIndex<struct FieldTag,FieldReflection>::Index;\
    }\
    template < size_t I>\
    static constexpr auto GetFieldName() noexcept{\
        return FieldReflection<I, struct FieldTag>::Name;\
    }\
    template < size_t I>\
    using FieldType_T = typename FieldReflection< I, struct FieldTag>::Type;\
    template <size_t I, class T>\
    static auto& GetField(T& Val) noexcept{\
        constexpr auto Ptr = FieldReflection<I, struct FieldTag>::template Offset<T>;\
        return Val.*Ptr;\
    }\
    template <size_t I, class T>\
    static const auto& GetField(const T& Val) noexcept {\
        constexpr auto Ptr = FieldReflection<I, struct FieldTag>::template Offset<T>; \
        return Val.*Ptr; \
    }\
    template<size_t I, class T, class F>\
    static void VisitSingleField(T& val, const F& f){\
        f(GetField<I,T>(val), GetFieldName<I>());\
    }\
    template<class F, class T, size_t ... Indices>\
    static void VisitFields(T& val, const F& f, std::index_sequence<Indices...>){\
        constexpr auto N = sizeof...(Indices);\
        int a[] = { (VisitSingleField<Indices,T,F>(val,f), 0) ... };\
        (void)a;(void)N;\
    }\
    template<class F, class T>\
    static void Visit(T& val, const F& f){\
        VisitFields<F, T>(val, f, std::make_index_sequence<GetNumFields()>());\
    }\
};

    