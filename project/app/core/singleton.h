#pragma once

#include <stdexcept>

namespace beryl
{
    template <typename T>
    class singleton // Use CTAD pattern
    {
    protected:
        inline static singleton* instance_ptr_ = nullptr;
        virtual ~singleton() { instance_ptr_ = nullptr; }
    public:
        singleton()
        {
            if (instance_ptr_ != nullptr)
                throw std::runtime_error("Unexpected multiple instances of a singleton class");
            instance_ptr_ = this;
        }
        singleton(const singleton&) = delete;
        singleton& operator=(const singleton&) = delete;
        singleton(singleton&&) = default;
        singleton& operator=(singleton&&) = default;
        static T& instance()
        {
            if (instance_ptr_ == nullptr)
                throw std::runtime_error("Instance is not yet constructed");
            static T* const ptr = dynamic_cast<T*>(instance_ptr_);
            return *ptr;
        }
    };
}