#include <iostream>
#include <memory>
#include <cstdlib>
#include <initializer_list>
#include <cmath>

template<typename T>
class PoolAllocator {
public:
    using value_type = T;
    using pointer = T*;
    using size_type = size_t;
    using difference_type = std::ptrdiff_t;
private:
    class Pool {
    private:
        size_type chunk_count_ = 0;
        size_type remain_size_ = 0;
        size_type chunk_size_ = 0;
        bool* chunks_ = nullptr;
    public:
        char* head_chunk_ = nullptr;

        Pool() = default;

        ~Pool() {
            free(chunks_);
            free(head_chunk_);
        }

        constexpr Pool(size_type chunk_size, size_type chunk_count)
                : chunk_size_(chunk_size),
                  chunk_count_(chunk_count),
                  remain_size_(chunk_count) {

            chunks_ = reinterpret_cast<bool*>(malloc(chunk_count));
            for (size_type i = 0; i < chunk_count; ++i) {
                chunks_[i] = false;
            }
            head_chunk_ = reinterpret_cast<char*>(malloc(chunk_size_ * chunk_count_));
        }

        size_type CheckPool(size_type amount_of_elements) {
            size_type begin = 0;
            size_type row_size = 0;
            bool current_row = false;

            for (size_type i = 0; i < chunk_count_; ++i) {
                if (!chunks_[i]) {
                    row_size += chunk_size_;
                    if (!current_row) {
                        current_row = true;
                        begin = i;
                    }

                    if (row_size >= amount_of_elements) {
                        for (size_type j = begin; j <= i; ++j) {
                            chunks_[j] = true;
                        }
                        remain_size_ -= (i - begin + 1);
                        return begin;
                    }
                } else {
                    row_size = 0;
                    current_row = false;
                }
            }
            throw std::bad_alloc();
        }

        void FreeChunk(size_type first_chunk, size_type amount_of_elements) {
            size_type amount_of_used_chunks = ceil(
                    static_cast<double>(amount_of_elements * sizeof(value_type)) / chunk_size_);
            for (size_type j = 0; j < amount_of_used_chunks; ++j) {
                chunks_[first_chunk + j] = false;
            }
            remain_size_ += amount_of_used_chunks;
        }

        bool InPool(pointer p) {
            return ((head_chunk_ <= reinterpret_cast<char*>(p)) &&
                    (reinterpret_cast<char*>(p) < head_chunk_ + chunk_count_ * chunk_size_));
        }

        size_type GetChunkCount() const {
            return chunk_count_;
        }

        size_type GetChunkSize() const {
            return chunk_size_;
        }

        size_type GetRemainSize() const {
            return remain_size_;
        }

        void SetRemainSize(int size) {
            remain_size_ = size;
        }

        bool GetFlag(size_type i) const {
            return chunks_[i];
        }

        void SetFlag(size_type i, bool answer) {
            chunks_[i] = answer;
        }
    };

    Pool** pools;
    size_type size_of_pools = 0;

public:
    constexpr PoolAllocator(const std::initializer_list<std::pair<size_type, size_type>>& blocks)
            : size_of_pools(blocks.size()) {
        pools = reinterpret_cast<Pool**>(malloc(blocks.size() * sizeof(Pool*)));
        size_type index = 0;
        for (auto block: blocks) {
            size_type size_of_chunks = block.first;
            size_type number_of_elements = block.second;
            pools[index] = new Pool(size_of_chunks, number_of_elements);
            ++index;
        }
    }

    ~PoolAllocator() {
        std::destroy_n(pools, size_of_pools);
    }

    constexpr pointer allocate(size_type amount_of_elements) {
        int answer_pool = -1;
        size_type bytes_to_use = amount_of_elements * sizeof(value_type);
        size_type minimum_bytes = SIZE_MAX;
        for (size_type i = 0; i < size_of_pools; ++i) {
            size_type current_bytes = pools[i]->GetRemainSize() * pools[i]->GetChunkSize();
            if (current_bytes >= bytes_to_use && current_bytes < minimum_bytes) {
                answer_pool = i;
                minimum_bytes = current_bytes;
            }
        }

        if (answer_pool == -1) {
            throw std::bad_alloc();
        } else {
            size_type ans = pools[answer_pool]->CheckPool(amount_of_elements * sizeof(value_type)) *
                            pools[answer_pool]->GetChunkSize();
            return reinterpret_cast<pointer>(pools[answer_pool]->head_chunk_ + ans);
        }
    }

    constexpr void deallocate(pointer p, size_type amount_of_elements) {
        bool flag = true;
        for (size_type i = 0; i < size_of_pools; ++i) {
            if (pools[i]->InPool(p)) {
                flag = false;
                size_type first_chunk = (reinterpret_cast<char*>(p) - pools[i]->head_chunk_) / pools[i]->GetChunkSize();
                pools[i]->FreeChunk(first_chunk, amount_of_elements);
            }
        }
        if (flag) {
            throw std::invalid_argument("Bad deallocation");
        }
    }
};
