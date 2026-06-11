#pragma once
#include <memory>
#include <inplace_vector>
#include <vector>
#include <optional>

namespace hx
{

template<class... Ts>
struct overloads : Ts... { using Ts::operator()...; };

template<typename T>
using uptr = std::unique_ptr<T>;

template<typename T>
using sptr = std::shared_ptr<T>;

template <typename T, size_t N>
class BlockVector
{
public:
    BlockVector()
    {
        elements.push_back(std::make_unique<std::inplace_vector<T, N>>());
    }
    
    T& push_back(const T& element)
    {
        auto push_opt = elements.back()->try_push_back(element);
        if (!push_opt.has_value())
        {
            elements.push_back(std::make_unique<std::inplace_vector<T, N>>());
            return push_back(element);
        }
        return push_opt.value();
    }
    
    T& at(size_t i)
    {
        size_t outer_i = i / N;
        size_t inner_i = i % N;

        return elements.at(outer_i).at(inner_i);
    }

    T& operator[](size_t i)
    {
        return at(i);
    }

    template <typename F>
    std::optional<T&> find_if(F condition)
    {
        for (auto& inplace : elements)
        {
            auto it = std::find_if(inplace->begin(), inplace->end(), condition);
            if (it != inplace->end()) return *it;
        }
        return std::nullopt;
    }

private:
    std::vector<uptr<std::inplace_vector<T, N>>> elements;
};

}

