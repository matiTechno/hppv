#pragma once

#include <cassert>
#include <functional>

namespace hppv
{

// when embedding in class with move semantics,
// always capture by value in the callback
class Deleter
{
public:
    Deleter() = default;
    ~Deleter() {clean();}
    Deleter(const Deleter&) = delete;
    Deleter& operator=(const Deleter&) = delete;

    Deleter(Deleter&& rhs):
        callback_(rhs.callback_)
    {
        rhs.callback_ = nullptr;
    }

    Deleter& operator=(Deleter&& rhs)
    {
        assert(this != &rhs);
        clean();
        callback_ = rhs.callback_;
        rhs.callback_ = nullptr;
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

    void clean() {if(callback_) callback_();}
};

} // namespace hppv
