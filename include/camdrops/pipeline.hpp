#pragma once
#ifndef __PIPELINE_HPP__
#define __PIPELINE_HPP__
#include <kafka/KafkaProducer.h>

namespace camdrops {
/**
 * @brief Configurations for the Kafka producer
 *  defaults are set and rest should come from env vars or cli args
 */
struct KafkaProducerConfig {
  std::string bootstrap_servers;
  std::string topic;
  std::string key_prefix;
  std::string compression_type = "lz4";
  bool enable_idempotence = true;
  std::string acks = "all";
};

struct CameraConfig {
  std::string camera_id;
  std::string camera_location;
  std::string rtsp_url;
  // std::string
};

class TrplBuffer {
 public:
 private:
  struct
}
}  // namespace camdrops

#endif  // __PIPELINE_HPP__