#include <iostream>
#include <thread>
#include <vector>
#include <chrono>
#include <moodycamel/concurrentqueue.h>
#include <kafka/KafkaProducer.h>


kafka::Properties get_kafka_props() {
    kafka::Properties props;
    props.put("bootstrap.servers", "localhost:9092");
    return props;
}

int main() {
    moodycamel::ConcurrentQueue<int> queue;

    std::thread metric_publisher([&queue]() {
        int item;
        while (true) {
            if (queue.try_dequeue(item)) {
                if (item == -1) break; // Poison pill
                std::cout << "Alive, item:" << item << std::endl;
            } else {
                std::cout << "Dead" << std::endl;
                std::this_thread::yield();
            }
        }
    });

    std::thread metric_publisher([&queue]() {
        int item;
        
        while (true) {
            if (queue.try_dequeue(item)) {
                if (item == -1) break; // Poison pill
                std::cout << "Alive, item:" << item << std::endl;
            } else {
                std::cout << "Dead" << std::endl;
                std::this_thread::yield();
            }
        }
    });
    
    return 0;
}