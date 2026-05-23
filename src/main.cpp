#include <camdrops/pipeline.hpp>
#include <blockingconcurrentqueue.h>
#include <atomic>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>
using namespace std::chrono_literals;

kafka::Properties get_kafka_props() {
  kafka::Properties props;
  const char* bootstrap_servers = std::getenv("KAFKA_BOOTSTRAP_SERVERS");
  if (bootstrap_servers == nullptr || bootstrap_servers[0] == '\0') {
    throw std::runtime_error(
        "KAFKA_BOOTSTRAP_SERVERS is not set. Example: export KAFKA_BOOTSTRAP_SERVERS=localhost:9092");
  }
  props.put("bootstrap.servers", bootstrap_servers);
  props.put("enable.idempotence", "true");
  props.put("acks", "all");
  props.put("compression.type", "lz4");
  return props;
}

int main() {
  std::cout << "Starting main thread" << std::endl;
  moodycamel::BlockingConcurrentQueue<int> queue;

  // std::thread metric_publisher([&queue]() {
  //   int item;
  //   while (true) {
  //     if (queue.wait_dequeue_timed(item, 500ms)) {
  //       if (item == -1) break;  // Poison pill
  //       std::cout << "Alive, item:" << item << std::endl;
  //     }
  //   }
  // });

  std::jthread kafka_publisher([&queue](std::stop_token stoken) {
    try {
      int item;

      std::string topic = "test_topic";
      std::string key = "frame_123";
      char value_buffer[256]; // Constant buffer to avoid dynamic allocations in the loop

      auto kafka_producer = std::make_unique<kafka::clients::producer::KafkaProducer>(get_kafka_props());

      auto deliveryCb = [](const kafka::clients::producer::RecordMetadata& metadata, const kafka::Error& error) {
        if (error) {
          std::cerr << "Failed to deliver message: " << error.message() << " metadata: " << metadata.toString() << std::endl;
        }
      };

      while (!stoken.stop_requested()) {
        if (queue.wait_dequeue_timed(item, 5ms)) {
          if (item == -1) break;  // Poison pill
          auto value_len = std::snprintf(value_buffer, sizeof(value_buffer) - 1, "Hello Kafka! Item: %d", item);
          value_buffer[value_len] = '\0';  // Ensure null-termination
          std::cout << "Publishing to Kafka, msg:\"" << value_buffer << '"' << std::endl;
          
          // Send the message
          // NOTE!: Sending to kafka is async call, so don't use local variables that might go out of scope
          auto record = kafka::clients::producer::ProducerRecord(topic, kafka::Key(key.c_str(), key.size()), kafka::Value(value_buffer, value_len));
          kafka_producer->send(record, deliveryCb);
        }
      }
      std::cout << "Exiting kafka publisher thread" << std::endl;
    } catch (const std::exception& ex) {
      std::cerr << "Kafka publisher failed: " << ex.what() << std::endl;
    }
  });

  int rc = 10;
  while (rc) {
    std::cout << "Simulating events: " << rc << std::endl;
    queue.enqueue(rc);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    rc--;
  }
  std::cout << "Sending poison pill explicitly" << std::endl;
  
  queue.enqueue(-1);  // Send poison pill to stop threads

  std::cout << "The version of cpp is used: " << __cplusplus << std::endl;
  std::cout << "Exiting main thread" << std::endl;
  return 0;
}
