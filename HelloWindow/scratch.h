#pragma once

// A helper for grabbing a memory address for structs written in-line,
// to pass "keyword args structs" to those C-compatible DirectX functions that must receive pointers.
// https://stackoverflow.com/a/47460052
// Alternatively:
//   - Abuse the C++20 std::data() overload with std::initializer_list, like so:
//     std::data({ STRUCT_TYPENAME { ... } })
//   - template<typename T> T& as_lvalue(T&& t) { return t; }
template<typename T, typename... Ts>
const T* args(Ts&& ...ts) {
	static const T t( std::forward<Ts>(ts)... );
	return &t;
}
template<typename T>
const T* args(const T&& t) { return &t; }
template<typename T> T& as_lvalue(T&& t) { return t; }
template<typename T> const T* const as_ptr(const T&& t) { return &t; }

