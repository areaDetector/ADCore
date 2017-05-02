#ifndef CIRCULAR_BUFFER_H_
#define CIRCULAR_BUFFER_H_

#include <algorithm>
#include <vector>

template <class T>
class CircularBuffer {
public:
    CircularBuffer(size_t max_length)
        : max_length_(max_length),
          cpos_(0),
          size_(0),
          buffer_(max_length)
    {}

    size_t max_size() const {
        return max_length_;
    }

    size_t size() const {
        return size_;
    }

    const T& first() const {
        return buffer_[cpos_];
    }

    const T& last() const {
        return buffer_[(cpos_ + max_length_ - 1) % max_length_];
    }

    void push_back(const T& el) {
        buffer_[cpos_] = el;

        cpos_ = (cpos_ + 1) % max_length_;

        if (size_ < max_length_) {
            size_++;
        }
    }

    const T& operator[](size_t i) const {
        return buffer_[(cpos_ + i) % size_];
    }

    void clear() {
        cpos_ = 0;
        size_ = 0;
    }

    const size_t copy_to_array(T * const buffer, size_t buffer_size) const {
        size_t size = std::min(size_, buffer_size);
        if (size_ < max_length_) {
            std::copy(buffer_.begin(), buffer_.begin() + size, buffer);
            return size;
        } else {
            size_t len_to_end = max_length_ - cpos_;
            if (size < len_to_end) {
                std::copy(buffer_.begin() + cpos_,
                        buffer_.begin() + cpos_ + size, buffer);
                return size;
            } else {
                std::copy(buffer_.begin() + cpos_,
                        buffer_.begin() + cpos_ + len_to_end,
                        buffer);
                std::copy(buffer_.begin(),
                        buffer_.begin() + (size - len_to_end),
                        buffer + len_to_end);
                return size;
            }
        }
    }

private:
    size_t max_length_;
    size_t cpos_;
    size_t size_;
    std::vector<T> buffer_;
};

#endif // CIRCULAR_BUFFER_H_
