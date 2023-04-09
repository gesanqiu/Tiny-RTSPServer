//
// Created by root on 4/9/23.
//

#ifndef PORTPOOL_H
#define PORTPOOL_H

#include <queue>
#include <set>
#include <mutex>

class PortPool {
public:
    PortPool(int start_port, int end_port) {
        for (int i = start_port; i <= end_port; i += 2) {
            available_ports_.push(i);
        }
    }

    // Allocate a pair of ports from the available ports
    int allocate_port_pair() {
        std::lock_guard<std::mutex> lock(ports_mutex_);

        if (available_ports_.empty()) {
            return -1;
        }

        int port_pair = available_ports_.front();
        available_ports_.pop();

        return port_pair;
    }

    // Release a pair of ports back to the available ports
    void release_port_pair(int port_pair) {
        std::lock_guard<std::mutex> lock(ports_mutex_);
        available_ports_.push(port_pair);
    }

private:
    std::queue<int> available_ports_;
    std::mutex ports_mutex_;
};


#endif //PORTPOOL_H
