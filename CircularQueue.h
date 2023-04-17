//
// Created by ts on 4/17/23.
//

#ifndef CIRCULAR_QUEUE_H
#define CIRCULAR_QUEUE_H

#include <vector>
#include <mutex>
#include <condition_variable>
#include <map>

template <typename T>
class CircularQueue {
public:
    CircularQueue(size_t capacity)
            : buffer_(capacity), write_position_(0) {}

    void set_exit() {
        std::unique_lock<std::mutex> lock(mutex_);
        exit_ = true;
        not_empty_.notify_all();
    }

    bool is_exit() {
        std::unique_lock<std::mutex> lock(mutex_);
        return exit_;
    }

    void produce(const T& item) {
        std::unique_lock<std::mutex> lock(mutex_);

        size_t next_position = (write_position_ + 1) % buffer_.size();

        // Wait until all consumers have consumed the data in the next slot
        while (is_position_in_use(next_position)) {
            not_full_.wait(lock);
        }

        buffer_[write_position_] = item;
        write_position_ = next_position;

        // Signal consumers that new data is available
        not_empty_.notify_all();
    }

    T consume(const std::string& session_id) {
        std::unique_lock<std::mutex> lock(mutex_);

        // Wait until there is data available for the consumer
        while (consumer_positions_[session_id] == write_position_ && !exit_) {
            not_empty_.wait(lock);
        }

        if (exit_) {
            return T();
        }

        T item = buffer_[consumer_positions_[session_id]];
        consumer_positions_[session_id] = (consumer_positions_[session_id] + 1) % buffer_.size();

        // Signal the producer that a slot is now available
        not_full_.notify_all();

        return item;
    }

    void register_consumer(const std::string& session_id) {
        std::unique_lock<std::mutex> lock(mutex_);
        consumer_positions_[session_id] = write_position_;
    }

    void unregister_consumer(const std::string& session_id) {
        std::unique_lock<std::mutex> lock(mutex_);
        consumer_positions_.erase(session_id);
    }

private:
    bool is_position_in_use(size_t position) {
        for (const auto& consumer : consumer_positions_) {
            if (consumer.second == position) {
                return true;
            }
        }
        return false;
    }

    std::vector<T> buffer_;
    size_t write_position_;
    std::map<std::string, size_t> consumer_positions_;
    bool exit_;

    std::mutex mutex_;
    std::condition_variable not_empty_;
    std::condition_variable not_full_;
};

#endif //CIRCULAR_QUEUE_H
