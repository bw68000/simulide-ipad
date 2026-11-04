#ifndef AS_SIM_WRAP_H
#define AS_SIM_WRAP_H
// AngelScript registration helpers.
// Desktop: native calling conventions (unchanged behaviour).
// WebAssembly: native calls are impossible, so every function is wrapped
// in an asCALL_GENERIC trampoline generated at compile time.
#include "angelscript.h"

#ifdef __EMSCRIPTEN__
#include <new>
#include <type_traits>
#include <utility>

namespace simas
{
    template<typename T>
    T arg( asIScriptGeneric* g, asUINT i )
    {
        using U = std::remove_cv_t<std::remove_reference_t<T>>;
        void* slot = g->GetAddressOfArg( i );
        if constexpr( std::is_reference_v<T> || std::is_class_v<U> )
            return *static_cast<U*>( *static_cast<void**>( slot ) ); // objects & refs are passed by pointer
        else
            return *static_cast<U*>( slot );                          // primitives & handles by value
    }

    template<typename R, typename F>
    void ret( asIScriptGeneric* g, F&& f )
    {
        if constexpr( std::is_void_v<R> ) f();
        else new( g->GetAddressOfReturnLocation() ) std::remove_cv_t<std::remove_reference_t<R>>( f() );
    }

    template<typename Sig, Sig fp> struct MW;

    template<typename C, typename R, typename... A, R (C::*fp)(A...)>
    struct MW<R (C::*)(A...), fp>
    {
        template<std::size_t... I>
        static void run( asIScriptGeneric* g, std::index_sequence<I...> )
        {
            C* o = static_cast<C*>( g->GetObject() );
            ret<R>( g, [&]() -> R { return (o->*fp)( arg<A>( g, I )... ); } );
        }
        static void call( asIScriptGeneric* g ) { run( g, std::index_sequence_for<A...>{} ); }
    };

    template<typename C, typename R, typename... A, R (C::*fp)(A...) const>
    struct MW<R (C::*)(A...) const, fp>
    {
        template<std::size_t... I>
        static void run( asIScriptGeneric* g, std::index_sequence<I...> )
        {
            C* o = static_cast<C*>( g->GetObject() );
            ret<R>( g, [&]() -> R { return (o->*fp)( arg<A>( g, I )... ); } );
        }
        static void call( asIScriptGeneric* g ) { run( g, std::index_sequence_for<A...>{} ); }
    };

    template<typename Sig, Sig fp> struct FW;

    template<typename R, typename... A, R (*fp)(A...)>
    struct FW<R (*)(A...), fp>
    {
        template<std::size_t... I>
        static void run( asIScriptGeneric* g, std::index_sequence<I...> )
        { ret<R>( g, [&]() -> R { return fp( arg<A>( g, I )... ); } ); }
        static void call( asIScriptGeneric* g ) { run( g, std::index_sequence_for<A...>{} ); }
    };
}

  // static_cast selects the overload and lets inherited methods bind to C
  #define SIM_AS_MPR(C,m,P,R) asFUNCTION((::simas::MW<R (C::*)P, static_cast<R (C::*)P>(&C::m)>::call)), asCALL_GENERIC
  #define SIM_AS_M(C,m)       asFUNCTION((::simas::MW<decltype(&C::m), &C::m>::call)), asCALL_GENERIC
  #define SIM_AS_F(f)         asFUNCTION((::simas::FW<decltype(&f), &f>::call)), asCALL_GENERIC
#else
  #define SIM_AS_MPR(C,m,P,R) asMETHODPR(C,m,P,R), asCALL_THISCALL
  #define SIM_AS_M(C,m)       asMETHOD(C,m), asCALL_THISCALL
  #define SIM_AS_F(f)         asFUNCTION(f), asCALL_CDECL
#endif
#endif
