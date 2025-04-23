#define HASH #
#define SLASH //
#define __F(x) x
#define STR(x) #x
#define INCLUDE(a) __F(HASH)include STR(a)

#define _PASTE_IMPL(a, b) a##b
#define _PASTE(a, b) _PASTE_IMPL(a, b)

// --- Internal Argument Processing Macros (Recursive) ---

#define _process_args0()
#define _process_args1(A) A
#define _process_args2(A, ...) A _process_args1(__VA_ARGS__)
#define _process_args3(A, ...) A _process_args2(__VA_ARGS__)
#define _process_args4(A, ...) A _process_args3(__VA_ARGS__)
#define _process_args5(A, ...) A _process_args4(__VA_ARGS__)
#define _process_args6(A, ...) A _process_args5(__VA_ARGS__)
#define _process_args7(A, ...) A _process_args6(__VA_ARGS__)
#define _process_args8(A, ...) A _process_args7(__VA_ARGS__)
#define _process_args9(A, ...) A _process_args8(__VA_ARGS__)
#define _process_args10(A, ...) A _process_args9(__VA_ARGS__)
#define _process_args11(A, ...) A _process_args10(__VA_ARGS__)
#define _process_args12(A, ...) A _process_args11(__VA_ARGS__)
#define _process_args13(A, ...) A _process_args12(__VA_ARGS__)
#define _process_args14(A, ...) A _process_args13(__VA_ARGS__)
#define _process_args15(A, ...) A _process_args14(__VA_ARGS__)
#define _process_args16(A, ...) A _process_args15(__VA_ARGS__)
#define _process_args17(A, ...) A _process_args16(__VA_ARGS__)
#define _process_args18(A, ...) A _process_args17(__VA_ARGS__)
#define _process_args19(A, ...) A _process_args18(__VA_ARGS__)
#define _process_args20(A, ...) A _process_args19(__VA_ARGS__)
#define _process_args21(A, ...) A _process_args20(__VA_ARGS__)
#define _process_args22(A, ...) A _process_args21(__VA_ARGS__)
#define _process_args23(A, ...) A _process_args22(__VA_ARGS__)
#define _process_args24(A, ...) A _process_args23(__VA_ARGS__)
#define _process_args25(A, ...) A _process_args24(__VA_ARGS__)
#define _process_args26(A, ...) A _process_args25(__VA_ARGS__)
#define _process_args27(A, ...) A _process_args26(__VA_ARGS__)
#define _process_args28(A, ...) A _process_args27(__VA_ARGS__)
#define _process_args29(A, ...) A _process_args28(__VA_ARGS__)
#define _process_args30(A, ...) A _process_args29(__VA_ARGS__)
#define _process_args31(A, ...) A _process_args30(__VA_ARGS__)
#define _process_args32(A, ...) A _process_args31(__VA_ARGS__)
#define _process_args33(A, ...) A _process_args32(__VA_ARGS__)
#define _process_args34(A, ...) A _process_args33(__VA_ARGS__)
#define _process_args35(A, ...) A _process_args34(__VA_ARGS__)
#define _process_args36(A, ...) A _process_args35(__VA_ARGS__)
#define _process_args37(A, ...) A _process_args36(__VA_ARGS__)
#define _process_args38(A, ...) A _process_args37(__VA_ARGS__)
#define _process_args39(A, ...) A _process_args38(__VA_ARGS__)
#define _process_args40(A, ...) A _process_args39(__VA_ARGS__)
#define _process_args41(A, ...) A _process_args40(__VA_ARGS__)
#define _process_args42(A, ...) A _process_args41(__VA_ARGS__)
#define _process_args43(A, ...) A _process_args42(__VA_ARGS__)
#define _process_args44(A, ...) A _process_args43(__VA_ARGS__)
#define _process_args45(A, ...) A _process_args44(__VA_ARGS__)
#define _process_args46(A, ...) A _process_args45(__VA_ARGS__)
#define _process_args47(A, ...) A _process_args46(__VA_ARGS__)
#define _process_args48(A, ...) A _process_args47(__VA_ARGS__)
#define _process_args49(A, ...) A _process_args48(__VA_ARGS__)
#define _process_args50(A, ...) A _process_args49(__VA_ARGS__)
#define _process_args51(A, ...) A _process_args50(__VA_ARGS__)
#define _process_args52(A, ...) A _process_args51(__VA_ARGS__)
#define _process_args53(A, ...) A _process_args52(__VA_ARGS__)
#define _process_args54(A, ...) A _process_args53(__VA_ARGS__)
#define _process_args55(A, ...) A _process_args54(__VA_ARGS__)
#define _process_args56(A, ...) A _process_args55(__VA_ARGS__)
#define _process_args57(A, ...) A _process_args56(__VA_ARGS__)
#define _process_args58(A, ...) A _process_args57(__VA_ARGS__)
#define _process_args59(A, ...) A _process_args58(__VA_ARGS__)
#define _process_args60(A, ...) A _process_args59(__VA_ARGS__)
#define _process_args61(A, ...) A _process_args60(__VA_ARGS__)
#define _process_args62(A, ...) A _process_args61(__VA_ARGS__)
#define _process_args63(A, ...) A _process_args62(__VA_ARGS__)
#define _process_args64(A, ...) A _process_args63(__VA_ARGS__)

// --- Argument Counting Macro ---
// --- (Keep the _nargs_seq and _nargs definitions matching the highest N) ---
#define _nargs_seq( \
    _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, \
    _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, \
    _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, \
    _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, \
    _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, \
    _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, \
    _61, _62, _63, _64, N, ...) N
#define _nargs(...) _nargs_seq(__VA_ARGS__, \
    64, 63, 62, 61, 60, 59, 58, 57, 56, 55, 54, 53, 52, 51, 50, \
    49, 48, 47, 46, 45, 44, 43, 42, 41, 40, 39, 38, 37, 36, 35, \
    34, 33, 32, 31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, \
    19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0)

// --- Argument Counting Macro ---
// --- (Keep the _nargs_seq and _nargs definitions matching the highest N) ---
#define _n0args_seq( \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, \
    _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, \
    _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, \
    _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, \
    _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, \
    _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, \
    _61, _62, _63, _64, N, ...) N
#define _n0args(...) _n0args_seq(__VA_ARGS__, \
    64, 63, 62, 61, 60, 59, 58, 57, 56, 55, 54, 53, 52, 51, 50, \
    49, 48, 47, 46, 45, 44, 43, 42, 41, 40, 39, 38, 37, 36, 35, \
    34, 33, 32, 31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, \
    19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0)

// --- Dispatcher ---
#define _process_vargs(N, ...) _process_args##N(__VA_ARGS__)
#define _process_dispatcher(N, ...) _process_vargs(N, __VA_ARGS__)
#define _process_args(...) _process_dispatcher(_nargs(__VA_ARGS__), __VA_ARGS__)
