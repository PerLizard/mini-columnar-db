
#pragma once

#include <cstddef>
#include <vector>

template <typename T>
class Column {
   public:
    void append(const T& value);

    const T& get(std::size_t index) const;

    std::size_t size() const noexcept;

    bool empty() const noexcept;

   private:
    std::vector<T> data_;
};

template <typename T>
void Column<T>::append(const T& value) {
    data_.push_back(value);
}
template <typename T>
const T& Column<T>::get(std::size_t index) const {
    return data_.at(index);
}
template <typename T>
std::size_t Column<T>::size() const noexcept {
    return data_.size();
}
template <typename T>
bool Column<T>::empty() const noexcept {
    return data_.empty();
}