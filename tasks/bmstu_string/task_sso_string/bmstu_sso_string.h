#pragma once

#include <algorithm>
#include <cassert>
#include <cstring>
#include <exception>
#include <iostream>
#include <sstream>
#include <type_traits>

namespace bmstu
{
template <typename T>
class basic_string;

using string = basic_string<char>;
using wstring = basic_string<wchar_t>;

template <typename T>
class basic_string
{
   private:
    static constexpr size_t LONG_STR_SIZE = sizeof(void*) * 3;
    static constexpr size_t SSO_CAPACITY = (LONG_STR_SIZE / sizeof(T)) - 1;

    struct LongString
    {
        T* ptr;
        size_t size;
        size_t capacity;
    };

    struct ShortString
    {
        T buffer[SSO_CAPACITY + 1];
    };

    union Data
    {
        LongString long_str;
        ShortString short_str;

        Data() : short_str{} {}
        ~Data() {}
    };

    Data data_;
    bool is_long_;
    unsigned char size_;  // Отдельно храним размер для коротких строк

   public:
    basic_string() : is_long_(false), size_(0)
    {
        data_.short_str.buffer[0] = T();
    }

    basic_string(size_t size)
    {
        is_long_ = size > SSO_CAPACITY;
        if (is_long_)
        {
            data_.long_str.ptr = new T[size + 1];
            data_.long_str.size = size;
            data_.long_str.capacity = size;
            size_ = 0;
            std::fill(data_.long_str.ptr, data_.long_str.ptr + size, T(' '));
            data_.long_str.ptr[size] = T();
        }
        else
        {
            size_ = static_cast<unsigned char>(size);
            std::fill(data_.short_str.buffer, data_.short_str.buffer + size,
                      T(' '));
            data_.short_str.buffer[size] = T();
        }
    }

    basic_string(std::initializer_list<T> il)
    {
        is_long_ = il.size() > SSO_CAPACITY;
        if (is_long_)
        {
            data_.long_str.ptr = new T[il.size() + 1];
            data_.long_str.size = il.size();
            data_.long_str.capacity = il.size();
            size_ = 0;
            std::copy(il.begin(), il.end(), data_.long_str.ptr);
            data_.long_str.ptr[il.size()] = T();
        }
        else
        {
            size_ = static_cast<unsigned char>(il.size());
            std::copy(il.begin(), il.end(), data_.short_str.buffer);
            data_.short_str.buffer[il.size()] = T();
        }
    }

    basic_string(const T* c_str)
    {
        size_t len = strlen_(c_str);
        is_long_ = len > SSO_CAPACITY;

        if (is_long_)
        {
            data_.long_str.ptr = new T[len + 1];
            data_.long_str.size = len;
            data_.long_str.capacity = len;
            size_ = 0;
            std::copy(c_str, c_str + len + 1, data_.long_str.ptr);
        }
        else
        {
            size_ = static_cast<unsigned char>(len);
            std::copy(c_str, c_str + len + 1, data_.short_str.buffer);
        }
    }

    basic_string(const basic_string& other)
        : is_long_(other.is_long_), size_(other.size_)
    {
        if (is_long_)
        {
            data_.long_str.ptr = new T[other.data_.long_str.capacity + 1];
            data_.long_str.size = other.data_.long_str.size;
            data_.long_str.capacity = other.data_.long_str.capacity;
            std::copy(other.data_.long_str.ptr,
                      other.data_.long_str.ptr + other.data_.long_str.size + 1,
                      data_.long_str.ptr);
        }
        else
        {
            std::copy(other.data_.short_str.buffer,
                      other.data_.short_str.buffer + SSO_CAPACITY + 1,
                      data_.short_str.buffer);
        }
    }

    basic_string(basic_string&& dying) noexcept
        : is_long_(dying.is_long_), size_(dying.size_)
    {
        if (is_long_)
        {
            data_.long_str.ptr = dying.data_.long_str.ptr;
            data_.long_str.size = dying.data_.long_str.size;
            data_.long_str.capacity = dying.data_.long_str.capacity;

            dying.data_.long_str.ptr = nullptr;
            dying.data_.long_str.size = 0;
            dying.data_.long_str.capacity = 0;
            dying.is_long_ = false;
            dying.size_ = 0;
        }
        else
        {
            std::copy(dying.data_.short_str.buffer,
                      dying.data_.short_str.buffer + SSO_CAPACITY + 1,
                      data_.short_str.buffer);

            dying.size_ = 0;
            dying.data_.short_str.buffer[0] = T();
        }
    }

    ~basic_string()
    {
        if (is_long_ && data_.long_str.ptr)
        {
            delete[] data_.long_str.ptr;
        }
    }

    const T* c_str() const
    {
        return is_long_ ? data_.long_str.ptr : data_.short_str.buffer;
    }

    size_t size() const
    {
        return is_long_ ? data_.long_str.size : static_cast<size_t>(size_);
    }

    bool is_using_sso() const { return !is_long_; }

    size_t capacity() const
    {
        return is_long_ ? data_.long_str.capacity : SSO_CAPACITY;
    }

    basic_string& operator=(basic_string&& other) noexcept
    {
        if (this != &other)
        {
            if (is_long_ && data_.long_str.ptr)
            {
                delete[] data_.long_str.ptr;
            }

            is_long_ = other.is_long_;
            size_ = other.size_;

            if (is_long_)
            {
                data_.long_str.ptr = other.data_.long_str.ptr;
                data_.long_str.size = other.data_.long_str.size;
                data_.long_str.capacity = other.data_.long_str.capacity;

                other.data_.long_str.ptr = nullptr;
                other.data_.long_str.size = 0;
                other.data_.long_str.capacity = 0;
                other.is_long_ = false;
                other.size_ = 0;
            }
            else
            {
                std::copy(other.data_.short_str.buffer,
                          other.data_.short_str.buffer + SSO_CAPACITY + 1,
                          data_.short_str.buffer);

                other.size_ = 0;
                other.data_.short_str.buffer[0] = T();
            }
        }
        return *this;
    }

    basic_string& operator=(const T* c_str)
    {
        size_t len = strlen_(c_str);

        if (len > SSO_CAPACITY)
        {
            if (!is_long_)
            {
                T* new_ptr = new T[len + 1];
                std::copy(data_.short_str.buffer,
                          data_.short_str.buffer + size_ + 1, new_ptr);
                data_.long_str.ptr = new_ptr;
                data_.long_str.size = len;
                data_.long_str.capacity = len;
                is_long_ = true;
                size_ = 0;
            }
            else if (data_.long_str.capacity < len)
            {
                T* new_ptr = new T[len + 1];
                std::copy(data_.long_str.ptr,
                          data_.long_str.ptr + data_.long_str.size + 1,
                          new_ptr);
                delete[] data_.long_str.ptr;
                data_.long_str.ptr = new_ptr;
                data_.long_str.capacity = len;
                data_.long_str.size = len;
            }
            else
            {
                data_.long_str.size = len;
            }
            std::copy(c_str, c_str + len + 1, data_.long_str.ptr);
        }
        else
        {
            if (is_long_)
            {
                delete[] data_.long_str.ptr;
                is_long_ = false;
            }
            size_ = static_cast<unsigned char>(len);
            std::copy(c_str, c_str + len + 1, data_.short_str.buffer);
        }

        return *this;
    }

    basic_string& operator=(const basic_string& other)
    {
        if (this != &other)
        {
            return operator=(other.c_str());
        }
        return *this;
    }

    friend basic_string<T> operator+(const basic_string<T>& left,
                                     const basic_string<T>& right)
    {
        size_t total_size = left.size() + right.size();
        basic_string<T> result;

        if (total_size > result.SSO_CAPACITY)
        {
            result.is_long_ = true;
            result.size_ = 0;
            result.data_.long_str.ptr = new T[total_size + 1];
            result.data_.long_str.size = total_size;
            result.data_.long_str.capacity = total_size;
        }
        else
        {
            result.is_long_ = false;
            result.size_ = static_cast<unsigned char>(total_size);
        }

        T* dest = result.is_long_ ? result.data_.long_str.ptr
                                  : result.data_.short_str.buffer;
        std::copy(left.c_str(), left.c_str() + left.size(), dest);
        std::copy(right.c_str(), right.c_str() + right.size(),
                  dest + left.size());
        dest[total_size] = T();

        return result;
    }

    basic_string& operator+=(const basic_string& other)
    {
        size_t old_size = size();
        size_t other_size = other.size();
        size_t new_size = old_size + other_size;

        if (new_size > SSO_CAPACITY)
        {
            if (!is_long_)
            {
                T* new_ptr = new T[new_size + 1];
                std::copy(data_.short_str.buffer,
                          data_.short_str.buffer + size_ + 1, new_ptr);
                data_.long_str.ptr = new_ptr;
                data_.long_str.size = new_size;
                data_.long_str.capacity = new_size;
                is_long_ = true;
                size_ = 0;
            }
            else if (data_.long_str.capacity < new_size)
            {
                T* new_ptr = new T[new_size + 1];
                std::copy(data_.long_str.ptr,
                          data_.long_str.ptr + data_.long_str.size + 1,
                          new_ptr);
                delete[] data_.long_str.ptr;
                data_.long_str.ptr = new_ptr;
                data_.long_str.capacity = new_size;
                data_.long_str.size = new_size;
            }
            else
            {
                data_.long_str.size = new_size;
            }

            std::copy(other.c_str(), other.c_str() + other_size,
                      data_.long_str.ptr + old_size);
            data_.long_str.ptr[new_size] = T();
        }
        else
        {
            if (is_long_)
            {
                T buffer[SSO_CAPACITY + 1];
                std::copy(data_.long_str.ptr,
                          data_.long_str.ptr + data_.long_str.size + 1, buffer);
                delete[] data_.long_str.ptr;
                is_long_ = false;
                std::copy(buffer, buffer + old_size + 1,
                          data_.short_str.buffer);
                size_ = static_cast<unsigned char>(old_size);
            }

            std::copy(other.c_str(), other.c_str() + other_size,
                      data_.short_str.buffer + old_size);
            size_ = static_cast<unsigned char>(new_size);
            data_.short_str.buffer[new_size] = T();
        }

        return *this;
    }

    basic_string& operator+=(T symbol)
    {
        size_t old_size = size();
        size_t new_size = old_size + 1;

        if (new_size > SSO_CAPACITY)
        {
            if (!is_long_)
            {
                T* new_ptr = new T[new_size + 1];
                std::copy(data_.short_str.buffer,
                          data_.short_str.buffer + size_ + 1, new_ptr);
                data_.long_str.ptr = new_ptr;
                data_.long_str.size = new_size;
                data_.long_str.capacity = new_size;
                is_long_ = true;
                size_ = 0;
            }
            else if (data_.long_str.capacity < new_size)
            {
                T* new_ptr = new T[new_size + 1];
                std::copy(data_.long_str.ptr,
                          data_.long_str.ptr + data_.long_str.size + 1,
                          new_ptr);
                delete[] data_.long_str.ptr;
                data_.long_str.ptr = new_ptr;
                data_.long_str.capacity = new_size;
                data_.long_str.size = new_size;
            }
            else
            {
                data_.long_str.size = new_size;
            }

            data_.long_str.ptr[old_size] = symbol;
            data_.long_str.ptr[new_size] = T();
        }
        else
        {
            if (is_long_)
            {
                T buffer[SSO_CAPACITY + 1];
                std::copy(data_.long_str.ptr,
                          data_.long_str.ptr + data_.long_str.size + 1, buffer);
                delete[] data_.long_str.ptr;
                is_long_ = false;
                std::copy(buffer, buffer + old_size + 1,
                          data_.short_str.buffer);
                size_ = static_cast<unsigned char>(old_size);
            }

            data_.short_str.buffer[old_size] = symbol;
            size_ = static_cast<unsigned char>(new_size);
            data_.short_str.buffer[new_size] = T();
        }

        return *this;
    }

    T& operator[](size_t index) noexcept
    {
        return is_long_ ? data_.long_str.ptr[index]
                        : data_.short_str.buffer[index];
    }

    const T& operator[](size_t index) const noexcept
    {
        return is_long_ ? data_.long_str.ptr[index]
                        : data_.short_str.buffer[index];
    }

    T& at(size_t index)
    {
        if (index >= size())
        {
            throw std::out_of_range("Wrong index");
        }
        return (*this)[index];
    }

    const T& at(size_t index) const
    {
        if (index >= size())
        {
            throw std::out_of_range("Wrong index");
        }
        return (*this)[index];
    }

    T* data() { return is_long_ ? data_.long_str.ptr : data_.short_str.buffer; }

    const T* data() const
    {
        return is_long_ ? data_.long_str.ptr : data_.short_str.buffer;
    }

   private:
    static size_t strlen_(const T* str)
    {
        if (!str)
            return 0;
        size_t len = 0;
        while (str[len] != T())
        {
            ++len;
        }
        return len;
    }
};

// Операторы ввода/вывода
inline std::ostream& operator<<(std::ostream& os, const string& str)
{
    return os << str.c_str();
}

inline std::wostream& operator<<(std::wostream& os, const wstring& str)
{
    return os << str.c_str();
}

inline std::istream& operator>>(std::istream& is, string& str)
{
    if (std::stringstream* ss = dynamic_cast<std::stringstream*>(&is))
    {
        str = ss->str().c_str();
    }
    else
    {
        std::string temp;
        std::getline(is, temp);
        str = temp.c_str();
    }
    return is;
}

inline std::wistream& operator>>(std::wistream& is, wstring& str)
{
    if (std::wstringstream* wss = dynamic_cast<std::wstringstream*>(&is))
    {
        str = wss->str().c_str();
    }
    else
    {
        std::wstring temp;
        std::getline(is, temp);
        str = temp.c_str();
    }
    return is;
}

}  // namespace bmstu