// #include <camdrops/frame_producer.hpp>
#include <blockingconcurrentqueue.h>
#include <kafka/KafkaProducer.h>

#include <chrono>
#include <cstdlib>
#include <atomic>
#include <iostream>
#include <string>
#include <thread>
#include <vector>
using namespace std::chrono_literals;

kafka::Properties get_kafka_props() {
  kafka::Properties props;
  props.put("bootstrap.servers", getenv("KAFKA_BOOTSTRAP_SERVERS"));
  props.put("enable.idempotence", "true");
  props.put("acks", "all");
  props.put("compression.type", "lz4");
  return props;
}

int main() {
  std::cout << "Starting main thread" << std::endl;
  moodycamel::BlockingConcurrentQueue<int> queue;
  std::atomic<bool> stop_requested{false};

  // std::thread metric_publisher([&queue]() {
  //   int item;
  //   while (true) {
  //     if (queue.wait_dequeue_timed(item, 500ms)) {
  //       if (item == -1) break;  // Poison pill
  //       std::cout << "Alive, item:" << item << std::endl;
  //     }
  //   }
  // });

  std::thread kafka_publisher([&queue, &stop_requested]() {
    int item;

    std::string topic = "test_topic";
    std::string key = "frame_123";

    auto kafka_producer = std::make_unique<kafka::clients::producer::KafkaProducer>(get_kafka_props());
    
    auto deliveryCb = [](const kafka::clients::producer::RecordMetadata& metadata, const kafka::Error& error) {
      if (error) {
        std::cerr << "Failed to deliver message: " << error.message() << std::endl;
      } else {
        const auto offset = metadata.offset();
        std::cout << "Message delivered to topic " << metadata.topic()
                  << "metadata:\"" << metadata.toString() << "\""
                  << " partition " << metadata.partition()
                  << " offset ";
        if (offset.has_value()) {
          std::cout << *offset;
        } else {
          std::cout << "<none>";
        }
        std::cout << std::endl;
      }
    };

    while (!stop_requested.load(std::memory_order_relaxed)) {
      if (queue.wait_dequeue_timed(item, 5ms)) {
        if (item == -1) break;  // Poison pill
        std::string value = "This is frame no: " + std::to_string(item);
        std::cout << "Publishing to Kafka, msg:\"" << value << '"' << std::endl;
        auto record = kafka::clients::producer::ProducerRecord(topic, kafka::Key(key.c_str(), key.size()), kafka::Value(value.c_str(), value.size()));

        // Send the message
        kafka_producer->send(record, deliveryCb);
      } else {
        // std::cout << "Nothing to send for 500ms" << std::endl;
      }
    }
    std::cout << "Exiting kafka publisher thread" << std::endl;
  });

  int rc = 10;
  while (rc) {
    std::cout << "Simulating events: " << rc << std::endl;
    queue.enqueue(rc);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    rc--;
  }
  std::cout << "Sending poison pill explicitly" << std::endl;
  stop_requested.store(true, std::memory_order_relaxed);
  queue.enqueue(-1);  // Send poison pill to stop threads

  // metric_publisher.join();
  kafka_publisher.join();
  std::cout << "Exiting main thread" << std::endl;
  return 0;
}
