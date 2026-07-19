#pragma once

// ptr.hpp — a friendlier pointer library for C++
// Header-only. Requires C++17 or later.
//
// Design goal: you should be able to write pointer-related code
// using plain English words instead of C++ symbols like & * -> new delete
//
// The symbols still exist inside this file's implementation — that's unavoidable
// in C++ — but you never have to write them yourself when using the library.

#include <memory>
#include <stdexcept>
#include <optional>
#include <functional>
#include <type_traits>
#include <utility>


// ═══════════════════════════════════════════════════════════════
//  PART 1 — ENGLISH KEYWORDS
//  Macros that replace cryptic C++ symbols with plain words.
// ═══════════════════════════════════════════════════════════════

// ── Reference / address-of ──────────────────────────────────────
// Instead of:  void foo(int& x)
// You write:   void foo(ref_to(int) x)
#define ref_to(T)           T&

// Instead of:  void foo(const int& x)
// You write:   void foo(readonly(int) x)
#define readonly(T)         const T&

// Instead of:  void foo(int* x)
// You write:   void foo(pointer_to(int) x)
#define pointer_to(T)       T*

// Instead of:  &some_variable
// You write:   address_of(some_variable)
#define address_of(x)       (&(x))

// Instead of:  *some_pointer
// You write:   value_at(some_pointer)
#define value_at(p)         (*(p))

// ── Move semantics ──────────────────────────────────────────────
// Instead of:  std::move(x)
// You write:   move_from(x)
#define move_from(x)        std::move(x)

// Instead of:  foo(std::move(bar))
// You write:   give_to(foo, bar)   [transfers ownership into a function call]
#define give_to(fn, x)      fn(std::move(x))

// ── Member access ───────────────────────────────────────────────
// Instead of:  ptr->member
// You write:   field(ptr, member)
#define field(ptr, member)  ((ptr)->member)

// ── Const ───────────────────────────────────────────────────────
// Instead of:  const int
// You write:   immutable(int)
#define immutable(T)        const T

// ── Null ────────────────────────────────────────────────────────
// Instead of:  nullptr
// You write:   nothing
#define nothing             nullptr

// ── Casting ─────────────────────────────────────────────────────
// Instead of:  static_cast<int>(x)
// You write:   as(int, x)
#define as(T, x)            static_cast<T>(x)

// Instead of:  reinterpret_cast<char*>(x)
// You write:   reinterpret(char*, x)
#define reinterpret(T, x)   reinterpret_cast<T>(x)

// ── Boolean words ───────────────────────────────────────────────
// Instead of:  !condition
// You write:   not_(condition)
#define not_(x)             (!(x))

// Instead of:  a && b
// You write:   both(a, b)
#define both(a, b)          ((a) && (b))

// Instead of:  a || b
// You write:   either(a, b)
#define either(a, b)        ((a) || (b))

// ── Function signature helpers ──────────────────────────────────
// Instead of:  int& foo(int& x, const std::string& s)
// You write:   returns(ref_to(int)) foo(ref_to(int) x, readonly(std::string) s)
#define returns(T)          T

// Instead of:  void
// You write:   nothing_returned  (optional style alias)
#define nothing_returned    void

// ── Dereference assignment ──────────────────────────────────────
// Instead of:  *p = value;
// You write:   write_to(p, value);
#define write_to(p, value)  ((*(p)) = (value))

// Instead of:  x = *p;
// You write:   x = read_from(p);
#define read_from(p)        (*(p))


// ═══════════════════════════════════════════════════════════════
//  PART 2 — POINTER TYPE ALIASES
//  Give pointer types readable English names.
// ═══════════════════════════════════════════════════════════════

// owned<T>     — you own this value on the heap; freed automatically
template<typename T>
using owned = std::unique_ptr<T>;

// shared<T>    — multiple owners; freed when the last owner is gone
template<typename T>
using shared = std::shared_ptr<T>;

// viewing<T>   — a non-owning look at something owned elsewhere (never null by convention)
template<typename T>
using viewing = T*;

// maybe<T>     — a pointer that might be null (makes the possibility explicit)
template<typename T>
using maybe = T*;

// owned_array<T> — owns a heap array; freed automatically
template<typename T>
using owned_array = std::unique_ptr<T[]>;


// ═══════════════════════════════════════════════════════════════
//  PART 3 — CREATION
// ═══════════════════════════════════════════════════════════════

// allocate<T>(args...)   — put a new T on the heap and own it
template<typename T, typename... Args>
owned<T> allocate(Args&&... args) {
    return std::make_unique<T>(std::forward<Args>(args)...);
}

// allocate_shared<T>(args...)  — put a new T on the heap, shared ownership
template<typename T, typename... Args>
shared<T> allocate_shared(Args&&... args) {
    return std::make_shared<T>(std::forward<Args>(args)...);
}

// allocate_array<T>(size)  — put an array of T on the heap and own it
template<typename T>
owned_array<T> allocate_array(std::size_t size) {
    return std::make_unique<T[]>(size);
}

// borrow(p)   — get a non-owning view of something you own
template<typename T>
viewing<T> borrow(const owned<T>& p) { return p.get(); }

template<typename T>
viewing<T> borrow(const shared<T>& p) { return p.get(); }

// empty<T>()  — a null/empty pointer of type T (use maybe<T> as the type)
template<typename T>
maybe<T> empty() { return nullptr; }


// ═══════════════════════════════════════════════════════════════
//  PART 4 — READING & WRITING THROUGH POINTERS
// ═══════════════════════════════════════════════════════════════

// read(p)          — get the value a pointer points to (throws if null)
template<typename T>
ref_to(T) read(const owned<T>& p) {
    if (!p) throw std::runtime_error("ptr: read() called on a null owned pointer");
    return *p;
}
template<typename T>
ref_to(T) read(const shared<T>& p) {
    if (!p) throw std::runtime_error("ptr: read() called on a null shared pointer");
    return *p;
}
template<typename T>
ref_to(T) read(viewing<T> p) {
    if (!p) throw std::runtime_error("ptr: read() called on a null viewing pointer");
    return *p;
}

// write(p, value)  — store a value through a pointer (throws if null)
template<typename T, typename U>
void write(const owned<T>& p, U&& value) {
    if (!p) throw std::runtime_error("ptr: write() called on a null owned pointer");
    *p = std::forward<U>(value);
}
template<typename T, typename U>
void write(const shared<T>& p, U&& value) {
    if (!p) throw std::runtime_error("ptr: write() called on a null shared pointer");
    *p = std::forward<U>(value);
}
template<typename T, typename U>
void write(viewing<T> p, U&& value) {
    if (!p) throw std::runtime_error("ptr: write() called on a null viewing pointer");
    *p = std::forward<U>(value);
}

// read_index(arr, i)   — read from a heap array at index i
template<typename T>
ref_to(T) read_index(const owned_array<T>& arr, std::size_t i) {
    return arr[i];
}

// write_index(arr, i, value)  — write into a heap array at index i
template<typename T, typename U>
void write_index(const owned_array<T>& arr, std::size_t i, U&& value) {
    arr[i] = std::forward<U>(value);
}


// ═══════════════════════════════════════════════════════════════
//  PART 5 — CHECKING POINTERS
// ═══════════════════════════════════════════════════════════════

// is_empty(p)  — true if the pointer holds nothing (is null)
template<typename T> bool is_empty(const owned<T>& p)  { return p == nullptr; }
template<typename T> bool is_empty(const shared<T>& p) { return p == nullptr; }
template<typename T> bool is_empty(maybe<T> p)         { return p == nullptr; }

// has_value(p) — true if the pointer points to something real
template<typename T> bool has_value(const owned<T>& p)  { return p != nullptr; }
template<typename T> bool has_value(const shared<T>& p) { return p != nullptr; }
template<typename T> bool has_value(maybe<T> p)         { return p != nullptr; }

// points_to_same(a, b)  — true if two pointers point at the exact same object
template<typename T, typename U>
bool points_to_same(T* a, U* b) { return static_cast<void*>(a) == static_cast<void*>(b); }


// ═══════════════════════════════════════════════════════════════
//  PART 6 — SAFE ACCESS PATTERNS
// ═══════════════════════════════════════════════════════════════

// read_or(p, fallback)
//   Returns the pointed-to value if non-null, otherwise returns fallback.
template<typename T>
T read_or(const owned<T>& p, T fallback) { return p ? *p : fallback; }

template<typename T>
T read_or(const shared<T>& p, T fallback) { return p ? *p : fallback; }

template<typename T>
T read_or(maybe<T> p, T fallback) { return p ? *p : fallback; }

// when_valid(p, fn)
//   Calls fn(value) only if the pointer is non-null.
//   Returns true if fn was called, false if the pointer was empty.
template<typename T, typename Fn>
bool when_valid(const owned<T>& p, Fn fn) {
    if (p) { fn(*p); return true; } return false;
}
template<typename T, typename Fn>
bool when_valid(const shared<T>& p, Fn fn) {
    if (p) { fn(*p); return true; } return false;
}
template<typename T, typename Fn>
bool when_valid(maybe<T> p, Fn fn) {
    if (p) { fn(*p); return true; } return false;
}

// when_empty(p, fn)
//   Calls fn() only if the pointer IS null. Good for "else" cases.
template<typename T, typename Fn>
bool when_empty(const owned<T>& p, Fn fn) {
    if (!p) { fn(); return true; } return false;
}
template<typename T, typename Fn>
bool when_empty(const shared<T>& p, Fn fn) {
    if (!p) { fn(); return true; } return false;
}
template<typename T, typename Fn>
bool when_empty(maybe<T> p, Fn fn) {
    if (!p) { fn(); return true; } return false;
}

// transform(p, fn)
//   If the pointer is valid, apply fn to its value and return the result
//   wrapped in std::optional. Returns std::nullopt if the pointer is null.
template<typename T, typename Fn>
auto transform(const owned<T>& p, Fn fn) -> std::optional<decltype(fn(*p))> {
    if (p) return fn(*p);
    return std::nullopt;
}
template<typename T, typename Fn>
auto transform(const shared<T>& p, Fn fn) -> std::optional<decltype(fn(*p))> {
    if (p) return fn(*p);
    return std::nullopt;
}
template<typename T, typename Fn>
auto transform(maybe<T> p, Fn fn) -> std::optional<decltype(fn(*p))> {
    if (p) return fn(*p);
    return std::nullopt;
}


// ═══════════════════════════════════════════════════════════════
//  PART 7 — TRANSFERRING OWNERSHIP
// ═══════════════════════════════════════════════════════════════

// hand_over(p)
//   Transfer ownership out of an owned<T>.
//   The original pointer becomes empty after this call.
template<typename T>
owned<T> hand_over(owned<T>& p) {
    return std::move(p);
}

// make_shared_ownership(p)
//   Convert an owned<T> into a shared<T> so multiple things can hold it.
template<typename T>
shared<T> make_shared_ownership(owned<T> p) {
    return shared<T>(std::move(p));
}

// release_raw(p)
//   Gives up ownership and returns a raw C++ pointer.
//   You are now fully responsible for deleting it. Use with care.
template<typename T>
viewing<T> release_raw(owned<T>& p) {
    return p.release();
}
