#pragma once

#include <cassert>
#include <functional> // std::function

namespace hppv
{

// when embedding in a class with move semantics,
// always capture by value in the callback

class Deleter
{
public:
    Deleter() = default;
    ~Deleter() {clean();}
    Deleter(const Deleter&) = delete;
    Deleter& operator=(const Deleter&) = delete;
    Deleter(Deleter&& rhs) {*this = std::move(rhs);}

    Deleter& operator=(Deleter&& rhs)
    {
        assert(this != &rhs);
        clean();
        callback_ = std::move(rhs.callback_);
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
