#pragma once

#include <stddef.h>
#include <stdint.h>

#include <math.h>
#include <new>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace std {

using ::size_t;
using ::ptrdiff_t;
using ::uint8_t;
using ::uint16_t;
using ::uint32_t;
using ::uint64_t;
using ::int8_t;
using ::int16_t;
using ::int32_t;
using ::int64_t;

template<class T>
struct remove_reference {
    using type = T;
};

template<class T>
struct remove_reference<T&> {
    using type = T;
};

template<class T>
struct remove_reference<T&&> {
    using type = T;
};

template<class T>
using remove_reference_t = typename remove_reference<T>::type;

template<class T>
constexpr T&& forward(remove_reference_t<T>& value) noexcept {
    return static_cast<T&&>(value);
}

template<class T>
constexpr T&& forward(remove_reference_t<T>&& value) noexcept {
    return static_cast<T&&>(value);
}

template<class T>
constexpr remove_reference_t<T>&& move(T&& value) noexcept {
    return static_cast<remove_reference_t<T>&&>(value);
}

template<class T>
void swap(T& left, T& right) {
    T saved = move(left);
    left = move(right);
    right = move(saved);
}

template<class A, class B>
struct pair {
    A first;
    B second;
    pair() = default;
    pair(const A& a, const B& b) : first(a), second(b) {}
    template<class U, class V>
    pair(U&& a, V&& b) : first(forward<U>(a)), second(forward<V>(b)) {}
};

template<class T>
struct less {
    bool operator()(const T& left, const T& right) const {
        return left < right;
    }
};

template<class T>
class initializer_list {
public:
    using iterator = const T*;
    constexpr initializer_list() : first_(nullptr), size_(0) {}
    constexpr initializer_list(const T* first, size_t size) : first_(first), size_(size) {}
    constexpr const T* begin() const { return first_; }
    constexpr const T* end() const { return first_ + size_; }
    constexpr size_t size() const { return size_; }

private:
    const T* first_;
    size_t size_;
};

template<class T>
constexpr const T* begin(initializer_list<T> list) {
    return list.begin();
}

template<class T>
constexpr const T* end(initializer_list<T> list) {
    return list.end();
}

template<class T>
class vector {
public:
    using iterator = T*;
    using const_iterator = const T*;
    using size_type = size_t;

    vector() : data_(nullptr), size_(0), capacity_(0) {}

    vector(size_t count) : data_(nullptr), size_(0), capacity_(0) {
        resize(count);
    }

    vector(size_t count, const T& value) : data_(nullptr), size_(0), capacity_(0) {
        resize(count, value);
    }

    template<class It>
    vector(It first, It last) : data_(nullptr), size_(0), capacity_(0) {
        for (; first != last; ++first) {
            push_back(*first);
        }
    }

    vector(initializer_list<T> list) : data_(nullptr), size_(0), capacity_(0) {
        reserve(list.size());
        for (const T* item = list.begin(); item != list.end(); ++item) {
            push_back(*item);
        }
    }

    vector(const vector& other) : data_(nullptr), size_(0), capacity_(0) {
        reserve(other.size_);
        for (size_t index = 0; index < other.size_; ++index) {
            push_back(other.data_[index]);
        }
    }

    vector(vector&& other) noexcept : data_(other.data_), size_(other.size_), capacity_(other.capacity_) {
        other.data_ = nullptr;
        other.size_ = 0;
        other.capacity_ = 0;
    }

    ~vector() {
        clear();
        free(data_);
    }

    vector& operator=(const vector& other) {
        if (this != &other) {
            clear();
            reserve(other.size_);
            for (size_t index = 0; index < other.size_; ++index) {
                push_back(other.data_[index]);
            }
        }
        return *this;
    }

    vector& operator=(vector&& other) noexcept {
        if (this != &other) {
            clear();
            free(data_);
            data_ = other.data_;
            size_ = other.size_;
            capacity_ = other.capacity_;
            other.data_ = nullptr;
            other.size_ = 0;
            other.capacity_ = 0;
        }
        return *this;
    }

    T& operator[](size_t index) { return data_[index]; }
    const T& operator[](size_t index) const { return data_[index]; }
    T& front() { return data_[0]; }
    const T& front() const { return data_[0]; }
    T& back() { return data_[size_ - 1]; }
    const T& back() const { return data_[size_ - 1]; }
    T* data() { return data_; }
    const T* data() const { return data_; }
    T* begin() { return data_; }
    const T* begin() const { return data_; }
    T* end() { return data_ + size_; }
    const T* end() const { return data_ + size_; }
    size_t size() const { return size_; }
    size_t capacity() const { return capacity_; }
    bool empty() const { return size_ == 0; }

    void clear() {
        if (!__is_trivially_destructible(T)) {
            for (size_t index = 0; index < size_; ++index) {
                data_[index].~T();
            }
        }
        size_ = 0;
    }

    void reserve(size_t count) {
        if (count <= capacity_) {
            return;
        }
        size_t nextCapacity = capacity_ == 0 ? 8 : capacity_;
        while (nextCapacity < count) {
            nextCapacity *= 2;
        }
        T* next = static_cast<T*>(malloc(sizeof(T) * nextCapacity));
        if (next == nullptr) {
            return;
        }
        if (__is_trivially_copyable(T)) {
            if (size_ > 0 && data_ != nullptr) {
                memcpy(next, data_, size_ * sizeof(T));
            }
        } else {
            for (size_t index = 0; index < size_; ++index) {
                new (next + index) T(move(data_[index]));
                data_[index].~T();
            }
        }
        free(data_);
        data_ = next;
        capacity_ = nextCapacity;
    }

    void push_back(const T& value) {
        reserve(size_ + 1);
        if (capacity_ < size_ + 1 || data_ == nullptr) {
            return;
        }
        new (data_ + size_) T(value);
        size_ += 1;
    }

    void push_back(T&& value) {
        reserve(size_ + 1);
        if (capacity_ < size_ + 1 || data_ == nullptr) {
            return;
        }
        new (data_ + size_) T(move(value));
        size_ += 1;
    }

    template<class... Args>
    T& emplace_back(Args&&... args) {
        reserve(size_ + 1);
        if (capacity_ < size_ + 1 || data_ == nullptr) {
            alignas(T) static uint8_t sink[sizeof(T)];
            return *reinterpret_cast<T*>(sink);
        }
        new (data_ + size_) T(forward<Args>(args)...);
        size_ += 1;
        return back();
    }

    void pop_back() {
        if (size_ == 0) {
            return;
        }
        size_ -= 1;
        data_[size_].~T();
    }

    void resize(size_t count) {
        if (count < size_) {
            if (__is_trivially_destructible(T)) {
                size_ = count;
                return;
            }
            while (size_ > count) {
                pop_back();
            }
            return;
        }
        reserve(count);
        if (capacity_ < count || data_ == nullptr) {
            return;
        }
        if (count > size_) {
            if (__is_trivially_constructible(T)) {
                memset(reinterpret_cast<uint8_t*>(data_ + size_), 0, (count - size_) * sizeof(T));
                size_ = count;
                return;
            }
            while (size_ < count) {
                new (data_ + size_) T();
                size_ += 1;
            }
        }
    }

    void resize(size_t count, const T& value) {
        if (count < size_) {
            resize(count);
            return;
        }
        reserve(count);
        if (capacity_ < count || data_ == nullptr) {
            return;
        }
        if (count > size_) {
            if (__is_trivially_copyable(T) && sizeof(T) == 1) {
                memset(reinterpret_cast<uint8_t*>(data_ + size_), static_cast<int>(reinterpret_cast<const uint8_t&>(value)),
                       count - size_);
                size_ = count;
                return;
            }
            while (size_ < count) {
                push_back(value);
            }
        }
    }

    iterator insert(iterator pos, const T& value) {
        size_t index = static_cast<size_t>(pos - data_);
        push_back(value);
        for (size_t cursor = size_ - 1; cursor > index; --cursor) {
            data_[cursor] = move(data_[cursor - 1]);
        }
        data_[index] = value;
        return data_ + index;
    }

    iterator erase(iterator pos) {
        size_t index = static_cast<size_t>(pos - data_);
        for (size_t cursor = index; cursor + 1 < size_; ++cursor) {
            data_[cursor] = move(data_[cursor + 1]);
        }
        pop_back();
        return data_ + index;
    }

    void assign(size_t count, const T& value) {
        clear();
        resize(count, value);
    }

    template<class It>
    void assign(It first, It last) {
        clear();
        for (; first != last; ++first) {
            push_back(*first);
        }
    }

    iterator erase(iterator first, iterator last) {
        size_t start = static_cast<size_t>(first - data_);
        size_t count = static_cast<size_t>(last - first);
        for (size_t step = 0; step < count; ++step) {
            erase(data_ + start);
        }
        return data_ + start;
    }

private:
    T* data_;
    size_t size_;
    size_t capacity_;
};

class string {
public:
    static constexpr size_t npos = static_cast<size_t>(-1);
    using iterator = char*;
    using const_iterator = const char*;

    string() : data_(nullptr), size_(0), capacity_(0) {}

    string(const char* text) : data_(nullptr), size_(0), capacity_(0) {
        assign(text, text == nullptr ? 0 : strlen(text));
    }

    string(const char* text, size_t count) : data_(nullptr), size_(0), capacity_(0) {
        assign(text, count);
    }

    string(size_t count, char value) : data_(nullptr), size_(0), capacity_(0) {
        resize(count, value);
    }

    string(const string& other) : data_(nullptr), size_(0), capacity_(0) {
        assign(other.data_, other.size_);
    }

    string(string&& other) noexcept : data_(other.data_), size_(other.size_), capacity_(other.capacity_) {
        other.data_ = nullptr;
        other.size_ = 0;
        other.capacity_ = 0;
    }

    ~string() { free(data_); }

    string& operator=(const string& other) {
        if (this != &other) {
            assign(other.data_, other.size_);
        }
        return *this;
    }

    string& operator=(string&& other) noexcept {
        if (this != &other) {
            free(data_);
            data_ = other.data_;
            size_ = other.size_;
            capacity_ = other.capacity_;
            other.data_ = nullptr;
            other.size_ = 0;
            other.capacity_ = 0;
        }
        return *this;
    }

    string& operator=(const char* text) {
        assign(text, text == nullptr ? 0 : strlen(text));
        return *this;
    }

    size_t size() const { return size_; }
    size_t length() const { return size_; }
    bool empty() const { return size_ == 0; }
    const char* c_str() const { return data_ == nullptr ? "" : data_; }
    const char* data() const { return c_str(); }
    char* data() { return data_; }
    char& operator[](size_t index) { return data_[index]; }
    const char& operator[](size_t index) const { return data_[index]; }
    char& front() { return data_[0]; }
    char& back() { return data_[size_ - 1]; }
    const char& back() const { return data_[size_ - 1]; }
    iterator begin() { return data_; }
    iterator end() { return data_ + size_; }
    const_iterator begin() const { return data_; }
    const_iterator end() const { return data_ + size_; }

    void clear() { size_ = 0; if (data_ != nullptr) data_[0] = 0; }

    void reserve(size_t count) {
        if (count + 1 <= capacity_) {
            return;
        }
        size_t next = capacity_ == 0 ? 16 : capacity_;
        while (next < count + 1) {
            next *= 2;
        }
        char* buffer = static_cast<char*>(malloc(next));
        if (size_ > 0 && data_ != nullptr) {
            memcpy(buffer, data_, size_);
        }
        buffer[size_] = 0;
        free(data_);
        data_ = buffer;
        capacity_ = next;
    }

    void resize(size_t count, char value = 0) {
        reserve(count);
        if (count > size_) {
            memset(data_ + size_, value, count - size_);
        }
        size_ = count;
        data_[size_] = 0;
    }

    void push_back(char value) {
        reserve(size_ + 1);
        data_[size_] = value;
        size_ += 1;
        data_[size_] = 0;
    }

    void pop_back() {
        if (size_ == 0) {
            return;
        }
        size_ -= 1;
        data_[size_] = 0;
    }

    string& append(const char* text, size_t count) {
        reserve(size_ + count);
        if (count > 0 && text != nullptr) {
            memcpy(data_ + size_, text, count);
        }
        size_ += count;
        data_[size_] = 0;
        return *this;
    }

    string& append(const string& other) { return append(other.c_str(), other.size_); }
    string& operator+=(const string& other) { return append(other); }
    string& operator+=(const char* text) { return append(text, text == nullptr ? 0 : strlen(text)); }
    string& operator+=(char value) { push_back(value); return *this; }

    string substr(size_t pos, size_t count = npos) const {
        if (pos > size_) {
            pos = size_;
        }
        if (count > size_ - pos) {
            count = size_ - pos;
        }
        return string(data_ + pos, count);
    }

    size_t find(char value, size_t pos = 0) const {
        for (size_t index = pos; index < size_; ++index) {
            if (data_[index] == value) {
                return index;
            }
        }
        return npos;
    }

    size_t find(const char* text, size_t pos = 0) const {
        if (text == nullptr) {
            return npos;
        }
        size_t count = strlen(text);
        if (count == 0) {
            return pos;
        }
        if (pos >= size_ || count > size_ - pos) {
            return npos;
        }
        for (size_t index = pos; index + count <= size_; ++index) {
            if (memcmp(data_ + index, text, count) == 0) {
                return index;
            }
        }
        return npos;
    }

    size_t find(const string& text, size_t pos = 0) const { return find(text.c_str(), pos); }

    size_t rfind(char value, size_t pos = npos) const {
        if (size_ == 0) {
            return npos;
        }
        size_t index = pos >= size_ ? size_ - 1 : pos;
        while (true) {
            if (data_[index] == value) {
                return index;
            }
            if (index == 0) {
                break;
            }
            index -= 1;
        }
        return npos;
    }

    int compare(const string& other) const {
        size_t count = size_ < other.size_ ? size_ : other.size_;
        int order = memcmp(c_str(), other.c_str(), count);
        if (order != 0) {
            return order;
        }
        if (size_ < other.size_) {
            return -1;
        }
        if (size_ > other.size_) {
            return 1;
        }
        return 0;
    }

    iterator insert(iterator pos, char value) {
        size_t index = data_ == nullptr || pos == nullptr ? 0 : static_cast<size_t>(pos - data_);
        insert(index, string(1, value));
        return data_ + index;
    }

    string& insert(size_t pos, const char* text) {
        if (pos > size_) {
            pos = size_;
        }
        string tail = substr(pos);
        size_ = pos;
        if (data_ != nullptr) {
            data_[size_] = 0;
        }
        append(text, text == nullptr ? 0 : strlen(text));
        append(tail);
        return *this;
    }

    string& insert(size_t pos, const string& text) { return insert(pos, text.c_str()); }

    string& erase(size_t pos = 0, size_t count = npos) {
        if (pos > size_) {
            pos = size_;
        }
        if (count > size_ - pos) {
            count = size_ - pos;
        }
        string tail = substr(pos + count);
        size_ = pos;
        if (data_ != nullptr) {
            data_[size_] = 0;
        }
        append(tail);
        return *this;
    }

    size_t find_first_not_of(const char* chars, size_t pos = 0) const {
        for (size_t index = pos; index < size_; ++index) {
            bool found = false;
            if (chars != nullptr) {
                for (size_t mark = 0; chars[mark] != 0; ++mark) {
                    if (data_[index] == chars[mark]) {
                        found = true;
                    }
                }
            }
            if (!found) {
                return index;
            }
        }
        return npos;
    }

    size_t find_last_not_of(const char* chars, size_t pos = npos) const {
        if (size_ == 0) {
            return npos;
        }
        size_t index = pos >= size_ ? size_ - 1 : pos;
        while (true) {
            bool found = false;
            if (chars != nullptr) {
                for (size_t mark = 0; chars[mark] != 0; ++mark) {
                    if (data_[index] == chars[mark]) {
                        found = true;
                    }
                }
            }
            if (!found) {
                return index;
            }
            if (index == 0) {
                break;
            }
            index -= 1;
        }
        return npos;
    }

    int compare(size_t pos, size_t count, const char* text) const {
        return substr(pos, count).compare(string(text));
    }

    int compare(size_t pos, size_t count, const string& text) const {
        return substr(pos, count).compare(text);
    }

    const char& front() const { return data_[0]; }

    void assign(size_t count, char value) { resize(count, value); }

    void assign(const char* text, size_t count) {
        reserve(count);
        if (count > 0 && text != nullptr) {
            memcpy(data_, text, count);
        }
        size_ = count;
        if (data_ != nullptr) {
            data_[size_] = 0;
        }
    }

private:
    char* data_;
    size_t size_;
    size_t capacity_;
};

inline bool operator==(const string& left, const string& right) { return left.compare(right) == 0; }
inline bool operator!=(const string& left, const string& right) { return !(left == right); }
inline bool operator<(const string& left, const string& right) { return left.compare(right) < 0; }
inline bool operator==(const string& left, const char* right) { return left.compare(string(right)) == 0; }
inline bool operator==(const char* left, const string& right) { return string(left) == right; }
inline bool operator!=(const string& left, const char* right) { return !(left == right); }
inline string operator+(const string& left, const string& right) { string out = left; out += right; return out; }
inline string operator+(const string& left, const char* right) { string out = left; out += right; return out; }
inline string operator+(const char* left, const string& right) { string out(left); out += right; return out; }
inline string operator+(const string& left, char right) { string out = left; out += right; return out; }

inline string to_string(int value) {
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "%d", value);
    return buffer;
}

inline string to_string(unsigned value) {
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "%u", value);
    return buffer;
}

inline string to_string(long value) {
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "%d", static_cast<int>(value));
    return buffer;
}

inline string to_string(double value) {
    char buffer[64];
    snprintf(buffer, sizeof(buffer), "%.6f", value);
    return buffer;
}

template<class T>
class unique_ptr {
public:
    unique_ptr() : pointer_(nullptr) {}
    explicit unique_ptr(T* pointer) : pointer_(pointer) {}
    unique_ptr(const unique_ptr&) = delete;
    unique_ptr(unique_ptr&& other) noexcept : pointer_(other.pointer_) { other.pointer_ = nullptr; }
    ~unique_ptr() { delete pointer_; }
    unique_ptr& operator=(unique_ptr&& other) noexcept {
        if (this != &other) {
            delete pointer_;
            pointer_ = other.pointer_;
            other.pointer_ = nullptr;
        }
        return *this;
    }
    T* get() const { return pointer_; }
    T* operator->() const { return pointer_; }
    T& operator*() const { return *pointer_; }
    explicit operator bool() const { return pointer_ != nullptr; }
    void reset(T* pointer = nullptr) {
        delete pointer_;
        pointer_ = pointer;
    }

private:
    T* pointer_;
};

template<class T, class... Args>
unique_ptr<T> make_unique(Args&&... args) {
    return unique_ptr<T>(new T(forward<Args>(args)...));
}

template<class T>
class shared_ptr {
public:
    shared_ptr() : pointer_(nullptr), count_(nullptr), free_(nullptr) {}
    explicit shared_ptr(T* pointer) : pointer_(pointer), count_(pointer == nullptr ? nullptr : new int(1)), free_(nullptr) {}
    shared_ptr(T* pointer, void (*freeObject)(void*))
        : pointer_(pointer), count_(pointer == nullptr ? nullptr : new int(1)), free_(freeObject) {}
    shared_ptr(const shared_ptr& other) : pointer_(other.pointer_), count_(other.count_), free_(other.free_) { add(); }
    shared_ptr(shared_ptr&& other) noexcept : pointer_(other.pointer_), count_(other.count_), free_(other.free_) {
        other.pointer_ = nullptr;
        other.count_ = nullptr;
        other.free_ = nullptr;
    }
    ~shared_ptr() { release(); }
    shared_ptr& operator=(const shared_ptr& other) {
        if (this != &other) {
            release();
            pointer_ = other.pointer_;
            count_ = other.count_;
            free_ = other.free_;
            add();
        }
        return *this;
    }
    shared_ptr& operator=(shared_ptr&& other) noexcept {
        if (this != &other) {
            release();
            pointer_ = other.pointer_;
            count_ = other.count_;
            free_ = other.free_;
            other.pointer_ = nullptr;
            other.count_ = nullptr;
            other.free_ = nullptr;
        }
        return *this;
    }
    T* get() const { return pointer_; }
    T* operator->() const { return pointer_; }
    T& operator*() const { return *pointer_; }
    explicit operator bool() const { return pointer_ != nullptr; }

private:
    void add() {
        if (count_ != nullptr) {
            *count_ += 1;
        }
    }
    void release() {
        if (count_ == nullptr) {
            return;
        }
        *count_ -= 1;
        if (*count_ == 0) {
            if (free_ != nullptr) {
                free_(pointer_);
            } else {
                delete pointer_;
            }
            delete count_;
        }
        pointer_ = nullptr;
        count_ = nullptr;
        free_ = nullptr;
    }
    T* pointer_;
    int* count_;
    void (*free_)(void*);
};

template<class T, class... Args>
shared_ptr<T> make_shared(Args&&... args) {
    return shared_ptr<T>(new T(forward<Args>(args)...));
}

template<class>
class function;

template<class R, class... Args>
class function<R(Args...)> {
    struct Base {
        virtual ~Base() = default;
        virtual R call(Args... args) = 0;
        virtual Base* clone() const = 0;
    };
    template<class F>
    struct Holder : Base {
        F function_;
        explicit Holder(F function) : function_(move(function)) {}
        R call(Args... args) override { return function_(forward<Args>(args)...); }
        Base* clone() const override { return new Holder(function_); }
    };

public:
    function() : holder_(nullptr) {}
    function(decltype(nullptr)) : holder_(nullptr) {}
    template<class F>
    function(F function) : holder_(new Holder<F>(move(function))) {}
    function(const function& other) : holder_(other.holder_ == nullptr ? nullptr : other.holder_->clone()) {}
    function(function&& other) noexcept : holder_(other.holder_) { other.holder_ = nullptr; }
    ~function() { delete holder_; }
    function& operator=(const function& other) {
        if (this != &other) {
            delete holder_;
            holder_ = other.holder_ == nullptr ? nullptr : other.holder_->clone();
        }
        return *this;
    }
    function& operator=(function&& other) noexcept {
        if (this != &other) {
            delete holder_;
            holder_ = other.holder_;
            other.holder_ = nullptr;
        }
        return *this;
    }
    R operator()(Args... args) const { return holder_->call(forward<Args>(args)...); }
    explicit operator bool() const { return holder_ != nullptr; }

private:
    Base* holder_;
};

template<class K, class V, class Cmp = less<K>>
class map {
public:
    using iterator = pair<K, V>*;
    using const_iterator = const pair<K, V>*;

    iterator begin() { return items_.begin(); }
    iterator end() { return items_.end(); }
    const_iterator begin() const { return items_.begin(); }
    const_iterator end() const { return items_.end(); }
    bool empty() const { return items_.empty(); }
    size_t size() const { return items_.size(); }
    size_t count(const K& key) const { return find(key) == end() ? 0u : 1u; }

    iterator find(const K& key) {
        for (size_t index = 0; index < items_.size(); ++index) {
            if (!(Cmp()(items_[index].first, key)) && !(Cmp()(key, items_[index].first))) {
                return items_.begin() + index;
            }
        }
        return end();
    }

    const_iterator find(const K& key) const {
        for (size_t index = 0; index < items_.size(); ++index) {
            if (!(Cmp()(items_[index].first, key)) && !(Cmp()(key, items_[index].first))) {
                return items_.begin() + index;
            }
        }
        return end();
    }

    V& operator[](const K& key) {
        iterator found = find(key);
        if (found != end()) {
            return found->second;
        }
        items_.push_back(pair<K, V>(key, V()));
        return items_.back().second;
    }

private:
    vector<pair<K, V>> items_;
};

template<class K>
struct hash {
    size_t operator()(const K& key) const { return static_cast<size_t>(key); }
};

template<>
struct hash<string> {
    size_t operator()(const string& key) const {
        size_t value = 1469598103934665603ull;
        for (size_t index = 0; index < key.size(); ++index) {
            value ^= static_cast<unsigned char>(key[index]);
            value *= 1099511628211ull;
        }
        return value;
    }
};

template<class K, class V, class Hash = hash<K>>
class unordered_map {
public:
    using iterator = pair<K, V>*;
    using const_iterator = const pair<K, V>*;

    iterator begin() { return items_.begin(); }
    iterator end() { return items_.end(); }
    const_iterator begin() const { return items_.begin(); }
    const_iterator end() const { return items_.end(); }
    bool empty() const { return items_.empty(); }
    size_t size() const { return items_.size(); }
    void clear() { items_.clear(); }

    iterator find(const K& key) {
        for (size_t index = 0; index < items_.size(); ++index) {
            if (!(key < items_[index].first) && !(items_[index].first < key)) {
                return items_.begin() + index;
            }
        }
        return end();
    }

    V& operator[](const K& key) {
        iterator found = find(key);
        if (found != end()) {
            return found->second;
        }
        items_.push_back(pair<K, V>(key, V()));
        return items_.back().second;
    }

    template<class KK, class VV>
    pair<iterator, bool> emplace(KK&& key, VV&& value) {
        iterator found = find(key);
        if (found != end()) {
            return pair<iterator, bool>(found, false);
        }
        items_.push_back(pair<K, V>(forward<KK>(key), forward<VV>(value)));
        return pair<iterator, bool>(items_.end() - 1, true);
    }

    iterator erase(iterator pos) {
        return items_.erase(pos);
    }

private:
    vector<pair<K, V>> items_;
};

template<class T>
struct char_traits {
    static size_t length(const T* text) {
        size_t count = 0;
        if (text == nullptr) {
            return 0;
        }
        while (text[count] != T{}) {
            count += 1;
        }
        return count;
    }
};

template<class T>
T min(initializer_list<T> list) {
    const T* cursor = list.begin();
    T best = *cursor;
    ++cursor;
    while (cursor != list.end()) {
        if (*cursor < best) {
            best = *cursor;
        }
        ++cursor;
    }
    return best;
}

template<class T>
T max(initializer_list<T> list) {
    const T* cursor = list.begin();
    T best = *cursor;
    ++cursor;
    while (cursor != list.end()) {
        if (best < *cursor) {
            best = *cursor;
        }
        ++cursor;
    }
    return best;
}

template<class T>
const T& min(const T& left, const T& right) {
    return right < left ? right : left;
}

template<class T>
const T& max(const T& left, const T& right) {
    return left < right ? right : left;
}

template<class T>
T clamp(const T& value, const T& low, const T& high) {
    return max(low, min(value, high));
}

template<class It, class Cmp>
void sort(It begin, It end, Cmp compare) {
    for (It cursor = begin; cursor != end; ++cursor) {
        It scan = cursor;
        while (scan != begin) {
            It previous = scan;
            --previous;
            if (!compare(*scan, *previous)) {
                break;
            }
            swap(*scan, *previous);
            scan = previous;
        }
    }
}

template<class It>
void sort(It begin, It end) {
    sort(begin, end, less<remove_reference_t<decltype(*begin)>>());
}

template<class It, class Cmp>
void stable_sort(It begin, It end, Cmp compare) {
    sort(begin, end, compare);
}

template<class It>
void reverse(It begin, It end) {
    while (begin != end) {
        --end;
        if (begin == end) {
            break;
        }
        swap(*begin, *end);
        ++begin;
    }
}

template<class In, class Out, class Fn>
Out transform(In begin, In end, Out dest, Fn function) {
    while (begin != end) {
        *dest = function(*begin);
        ++begin;
        ++dest;
    }
    return dest;
}

using nullptr_t = decltype(nullptr);

inline int abs(int value) { return value < 0 ? -value : value; }
inline float abs(float value) { return fabsf(value); }
inline double abs(double value) { return ::fabs(value); }
inline double atof(const char* text) {
    if (text == nullptr) {
        return 0.0;
    }
    while (*text == ' ' || *text == '\t') {
        text += 1;
    }
    int sign = 1;
    if (*text == '-') {
        sign = -1;
        text += 1;
    }
    double value = 0.0;
    while (*text >= '0' && *text <= '9') {
        value = value * 10.0 + (*text - '0');
        text += 1;
    }
    if (*text == '.') {
        text += 1;
        double place = 0.1;
        while (*text >= '0' && *text <= '9') {
            value += (*text - '0') * place;
            place *= 0.1;
            text += 1;
        }
    }
    return value * sign;
}
inline float round(float value) { return static_cast<float>(value >= 0.0f ? floorf(value + 0.5f) : ceilf(value - 0.5f)); }
inline double round(double value) { return value >= 0.0 ? ::floor(value + 0.5) : ::ceil(value - 0.5); }
inline double pow(double base, double exponent) { return ::pow(base, exponent); }
inline float pow(float base, float exponent) { return static_cast<float>(::pow(base, exponent)); }
inline double fmod(double value, double divisor) { return ::fmod(value, divisor); }
inline double cos(double value) { return ::cos(value); }
inline float cos(float value) { return static_cast<float>(::cos(value)); }
inline double sin(double value) { return ::cos(value - 1.5707963267948966); }
inline float sin(float value) { return static_cast<float>(sin(static_cast<double>(value))); }
inline double exp(double value) {
    double sum = 1.0;
    double term = 1.0;
    for (int step = 1; step < 24; ++step) {
        term *= value / static_cast<double>(step);
        sum += term;
    }
    return sum;
}
inline float exp(float value) { return static_cast<float>(exp(static_cast<double>(value))); }
inline bool isfinite(float value) { return value == value && value < 1.0e30f && value > -1.0e30f; }
inline bool isfinite(double value) { return value == value && value < 1.0e300 && value > -1.0e300; }
inline float floor(float value) { return floorf(value); }
inline double floor(double value) { return ::floor(value); }
inline float ceil(float value) { return ceilf(value); }
inline double ceil(double value) { return ::ceil(value); }
inline float fabs(float value) { return fabsf(value); }
inline double fabs(double value) { return ::fabs(value); }
inline float sqrt(float value) { return sqrtf(value); }
inline double sqrt(double value) { return ::sqrt(value); }
inline long lround(float value) { return ::lroundf(value); }
inline long lround(double value) { return ::lround(value); }
int snprintf(char* buffer, size_t capacity, const char* format, ...);

inline int atoi(const char* text) { return ::atoi(text); }

}  // namespace std
