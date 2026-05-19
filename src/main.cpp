#include <blockingconcurrentqueue.h>
#include <kafka/KafkaProducer.h>

#include <chrono>
#include <iostream>
#include <thread>
#include <vector>
using namespace std::chrono_literals;

kafka::Properties get_kafka_props() {
  kafka::Properties props;
  props.put("bootstrap.servers", "kafka:9092");
  return props;
}

int main() {
  std::cout << "Starting main thread" << std::endl;
  moodycamel::BlockingConcurrentQueue<int> queue;

  std::thread metric_publisher([&queue]() {
    int item;
    while (true) {
      if (queue.wait_dequeue_timed(item, 500ms)) {
        if (item == -1) break;  // Poison pill
        std::cout << "Alive, item:" << item << std::endl;
      }
    }
  });

  // std::thread kafka_publisher([&queue]() {
  //   int item;

  //   while (true) {
  //     if (queue.try_dequeue(item)) {
  //       if (item == -1) break;  // Poison pill
  //       std::cout << "Alive, item:" << item << std::endl;
  //     } else {
  //       std::cout << "Dead" << std::endl;
  //       std::this_thread::sleep_for(std::chrono::milliseconds(500));
  //     }
  //   }
  // });

  int rc = 10;
  while(rc) {
    std::cout << "Simulating events: " << rc << std::endl;
    queue.enqueue(rc);
    std::this_thread::sleep_for(std::chrono::microseconds(10));
    rc--;
  }
  std::cout << "Sending poison pill explicitly" << std::endl;
  queue.enqueue(-1);  // Send poison pill to stop threads

  metric_publisher.join();
  // kafka_publisher.join();
  std::cout << "Exiting main thread" << std::endl;
  return 0;
}