/*
 * C++ Standard Functions
 *
 * Copyright (C) 2019-2026 Udo Steinberg, BlueRock Security, Inc.
 *
 * This file is part of the NOVA microhypervisor.
 *
 * NOVA is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * NOVA is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License version 2 for more details.
 */

#pragma once

#include "types.hpp"

namespace std
{
    // See https://en.cppreference.com/w/cpp/types/conditional
    template<bool B, typename T, typename F> struct conditional {};
    template<        typename T, typename F> struct conditional<true,  T, F> { using type = T; };
    template<        typename T, typename F> struct conditional<false, T, F> { using type = F; };
    template<bool B, typename T, typename F> using  conditional_t = typename conditional<B, T, F>::type;

    // See https://en.cppreference.com/w/cpp/types/remove_cv
    template<typename T> struct remove_cv                   { using type = T; };
    template<typename T> struct remove_cv<T const>          { using type = T; };
    template<typename T> struct remove_cv<T volatile>       { using type = T; };
    template<typename T> struct remove_cv<T const volatile> { using type = T; };
    template<typename T> using  remove_cv_t = typename remove_cv<T>::type;

    // See https://en.cppreference.com/w/cpp/types/remove_reference
    template<typename T> struct remove_reference            { using type = T; };    // non-reference
    template<typename T> struct remove_reference<T&>        { using type = T; };    // lvalue reference
    template<typename T> struct remove_reference<T&&>       { using type = T; };    // rvalue reference
    template<typename T> using  remove_reference_t = typename remove_reference<T>::type;

    // See https://en.cppreference.com/w/cpp/utility/forward
    template<typename T> [[nodiscard]] constexpr T&& forward (remove_reference_t<T>&  t) noexcept { return static_cast<T&&>(t); }
    template<typename T> [[nodiscard]] constexpr T&& forward (remove_reference_t<T>&& t) noexcept { return static_cast<T&&>(t); }

    // See https://en.cppreference.com/w/cpp/utility/launder
    template<typename T> [[nodiscard]] constexpr T* launder (T* t) noexcept { return __builtin_launder (t); }

    // See https://en.cppreference.com/w/cpp/utility/move
    template<typename T> [[nodiscard]] constexpr remove_reference_t<T>&& move (T&& t) noexcept { return static_cast<remove_reference_t<T>&&>(t); }

    // See https://en.cppreference.com/w/cpp/algorithm/swap
    template<typename T> constexpr void swap (T& a, T& b) noexcept { T t { move (a) }; a = move (b); b = move (t); }

    // See https://en.cppreference.com/w/cpp/utility/to_underlying
    template<typename T> [[nodiscard]] constexpr auto to_underlying (T t) noexcept { return static_cast<__underlying_type (T)>(t); }

    // See https://en.cppreference.com/w/cpp/types/integral_constant
    template<typename T, T v> struct integral_constant
    {
        static constexpr T value { v };
        using type = integral_constant<T, v>;
    };

    using true_type  = integral_constant<bool, true>;
    using false_type = integral_constant<bool, false>;

    // See https://en.cppreference.com/w/cpp/types/is_integral
    template<typename>   struct __helper_int                        : false_type {};
    template<>           struct __helper_int<bool>                  : true_type  {};
    template<>           struct __helper_int<char>                  : true_type  {};
    template<>           struct __helper_int<signed char>           : true_type  {};
    template<>           struct __helper_int<signed short>          : true_type  {};
    template<>           struct __helper_int<signed int>            : true_type  {};
    template<>           struct __helper_int<signed long>           : true_type  {};
    template<>           struct __helper_int<signed long long>      : true_type  {};
    template<>           struct __helper_int<unsigned char>         : true_type  {};
    template<>           struct __helper_int<unsigned short>        : true_type  {};
    template<>           struct __helper_int<unsigned int>          : true_type  {};
    template<>           struct __helper_int<unsigned long>         : true_type  {};
    template<>           struct __helper_int<unsigned long long>    : true_type  {};
    template<typename T> struct is_integral : __helper_int<remove_cv_t<T>>::type {};

    template<typename T> concept integral = is_integral<T>::value;

    // See https://en.cppreference.com/w/cpp/types/is_pointer
    template<typename>   struct __helper_ptr                        : false_type {};
    template<typename T> struct __helper_ptr<T*>                    : true_type  {};
    template<typename T> struct is_pointer : __helper_ptr<remove_cv_t<T>>::type  {};

    template<typename T> concept pointer = is_pointer<T>::value;

    // See https://en.cppreference.com/w/cpp/types/is_reference
    template<typename T> struct is_reference                        : false_type {};
    template<typename T> struct is_reference<T&>                    : true_type  {};
    template<typename T> struct is_reference<T&&>                   : true_type  {};

    template<typename T> concept reference = is_reference<T>::value;

    // See https://en.cppreference.com/w/cpp/types/is_lvalue_reference
    template<typename>   struct is_lvalue_reference                 : false_type {};
    template<typename T> struct is_lvalue_reference<T&>             : true_type  {};

    template<typename T> concept lvalue_reference = is_lvalue_reference<T>::value;

    // See https://en.cppreference.com/w/cpp/types/is_rvalue_reference
    template<typename>   struct is_rvalue_reference                 : false_type {};
    template<typename T> struct is_rvalue_reference<T&&>            : true_type  {};

    template<typename T> concept rvalue_reference = is_rvalue_reference<T>::value;

    // See https://en.cppreference.com/w/cpp/types/is_same
    template<typename T, typename U> struct is_same                 : false_type {};
    template<typename T>             struct is_same<T, T>           : true_type  {};

    template<typename T, typename U> concept same_as = is_same<T, U>::value && is_same<U, T>::value;
}

// See https://en.cppreference.com/w/cpp/memory/new/operator_new
[[nodiscard]] inline void *operator new   (size_t, void *p) noexcept { return p; }
[[nodiscard]] inline void *operator new[] (size_t, void *p) noexcept { return p; }
