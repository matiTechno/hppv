#pragma once

#include <cassert>
#include <functional>

// when embedding in class with move semantics
// always capture by value in the callback
class Deleter
{
public:
    Deleter() = default;
    ~Deleter() {if(callback_) callback_();}
    Deleter(const Deleter&) = delete;
    Deleter& operator=(const Deleter&) = delete;

    Deleter(Deleter&& rhs):
        callback_(rhs.callback_)
    {
        rhs.callback_ = nullptr;
    }

    Deleter& operator=(Deleter&& rhs)
    {
        if(this != &rhs)
        {
            this->~Deleter();
            callback_ = rhs.callback_;
            rhs.callback_ = nullptr;
        }
        return *this;
    }

    template<typename T>
    void set(T&& callback)
    {
        assert(!callback_);
        callback_ = std::forward<T>(callback);
    }

private:
    std::function<void(void)> callback_;
};
