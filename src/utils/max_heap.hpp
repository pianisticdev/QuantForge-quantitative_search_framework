#ifndef QUANT_FORGE_MAX_HEAP_HPP
#define QUANT_FORGE_MAX_HEAP_HPP

#include <optional>
#include <vector>

namespace data_structures {
    template <typename T>
    class MaxHeap {
       public:
        MaxHeap() = default;
        ~MaxHeap() = default;
        MaxHeap(const MaxHeap&) = delete;
        MaxHeap& operator=(const MaxHeap&) = delete;
        MaxHeap(MaxHeap&&) = delete;
        MaxHeap& operator=(MaxHeap&&) = delete;

        void push(const T& value) {
            data_.emplace_back(value);
            bubble_up(data_.size() - 1);
        };

        void pop() {
            if (data_.empty()) {
                return;
            }

            data_[0] = std::move(data_.back());
            data_.pop_back();

            if (!data_.empty()) {
                bubble_down(0);
            }
        }

        [[nodiscard]] std::optional<T> top() const {
            if (!data_.empty()) {
                return data_.front();
            }

            return std::nullopt;
        };

        [[nodiscard]] bool empty() const { return data_.empty(); }

        [[nodiscard]] size_t size() const { return data_.size(); }

        void bubble_up(size_t idx) {
            while (idx > 0) {
                size_t parent_idx = (idx - 1) / 2;
                if (data_[idx] > data_[parent_idx]) {
                    std::swap(data_[idx], data_[parent_idx]);
                    idx = parent_idx;
                } else {
                    break;
                }
            }
        }

        void bubble_down(size_t idx) {
            size_t size = data_.size();

            while (true) {
                size_t left_child_idx = 2 * idx + 1;
                size_t right_child_idx = 2 * idx + 2;
                size_t largest_idx = idx;

                if (left_child_idx < size && data_[left_child_idx] > data_[largest_idx]) {
                    largest_idx = left_child_idx;
                }

                if (right_child_idx < size && data_[right_child_idx] > data_[largest_idx]) {
                    largest_idx = right_child_idx;
                }

                if (largest_idx == idx) {
                    break;
                }

                std::swap(data_[idx], data_[largest_idx]);
                idx = largest_idx;
            }
        }

       private:
        std::vector<T> data_;
    };
}  // namespace data_structures

#endif
