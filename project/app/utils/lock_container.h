#pragma once

#include <mutex>
#include <shared_mutex>
#include <vector>
#include <map>
#include <set>

namespace beryl
{
    template <typename DataType>
    class GuardedData final
    {
    public:
        class locked_data final
        {
            friend class GuardedData;
        private:
            std::unique_lock<std::shared_mutex> lock_;
            DataType* data_;
            DataType* accessed_data_;
            locked_data(std::shared_mutex& mutex, DataType& data) :
                lock_(mutex), data_(std::addressof(data)), accessed_data_(data_) {}
        public:
            DataType& get() const { return *accessed_data_; }
            DataType& operator*() const { return *accessed_data_; }
            DataType* operator->() const { return accessed_data_; }
            template <typename Index>
            auto& operator[](Index&& index) const { return (*accessed_data_)[std::forward<Index>(index)]; }
            template <typename... Types>
            auto& operator()(Types&&... args) const { return (*accessed_data_)(std::forward<Types>(args)...); }
            void unlock()
            {
                accessed_data_ = nullptr;
                lock_.unlock();
            }
            void lock()
            {
                accessed_data_ = data_;
                lock_.lock();
            }
        };
        class SharedLockedData final
        {
            friend class GuardedData;
        private:
            std::shared_lock<std::shared_mutex> lock_;
            const DataType* data_;
            SharedLockedData(std::shared_mutex& mutex, const DataType& data) :
                lock_(mutex), data_(std::addressof(data)) {}
        public:
            const DataType& get() const { return *data_; }
            const DataType& operator*() const { return *data_; }
            const DataType* operator->() const { return data_; }
            template <typename Index>
            auto operator[](Index&& index) const { return (*data_)[std::forward<Index>(index)]; }
            template <typename... Types>
            auto operator()(Types&&... args) const { return (*data_)(std::forward<Types>(args)...); }
            void unlock() { lock_.unlock(); }
            void lock() { lock_.lock(); }
        };
    private:
        mutable std::shared_mutex mutex_;
        DataType data_;
    public:
        template <typename... Types>
        explicit GuardedData(Types&&... args) : data_(std::forward<Types>(args)...) {}
        template <typename ElementType>
        GuardedData(const std::initializer_list<ElementType> list) : data_{ list } {}
        GuardedData(const GuardedData& other) { *this = other; }
        GuardedData(GuardedData&& other) noexcept { *this = std::move(other); }
        GuardedData& operator=(const GuardedData& other)
        {
            if (std::addressof(other) == this) return *this;
            std::unique_lock this_lock(mutex_);
            std::shared_lock other_lock(other.mutex_);
            data_ = other.data_;
            return *this;
        }
        GuardedData& operator=(GuardedData&& other) noexcept
        {
            if (std::addressof(other) == this) return *this;
            std::unique_lock this_lock(mutex_);
            std::unique_lock other_lock(other.mutex_);
            data_ = std::move(other.data_);
            return *this;
        }
        locked_data to_write() { return { mutex_, data_ }; }
        SharedLockedData to_read() const { return { mutex_, data_ }; }
        locked_data operator->() { return to_write(); }
        SharedLockedData operator->() const { return to_read(); }
    };

    namespace lock
    {
        template <typename T> using vector = GuardedData<std::vector<T>>;
        template <typename K, typename V> using map = GuardedData<std::map<K, V>>;
        template <typename T> using set = GuardedData<std::set<T>>;
    }
}
