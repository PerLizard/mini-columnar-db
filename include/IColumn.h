
#pragma once

#include <cstddef>

class IColumn {
   public:
    virtual ~IColumn() = default;

    virtual std::size_t size() const noexcept = 0;
    virtual bool empty() const noexcept = 0;
    virtual void popBack() noexcept = 0;
};
