#ifndef MODULES_CONGESTION_CONTROLLER_GOOG_CC_GCC_METRICS_LOGGER_H_
#define MODULES_CONGESTION_CONTROLLER_GOOG_CC_GCC_METRICS_LOGGER_H_

#include <fstream>
#include <mutex>
#include <string>

#include "api/transport/bandwidth_usage.h"
#include "api/units/data_rate.h"
#include "api/units/timestamp.h"
#include "rtc_base/synchronization/mutex.h"
#include "rtc_base/thread_annotations.h"

namespace webrtc {

// Singleton class that logs GCC metrics to a CSV file
class GccMetricsLogger {
 public:
  static GccMetricsLogger* GetInstance();
  
  // Initialize the logger with a file path
  void Initialize(const std::string& file_path);
  
  // Close the log file
  void Close();
  
  // Set the peer ID for logging purposes
  void SetPeerId(int peer_id);
  
  // Log trendline metrics
  void LogTrendlineMetrics(
      Timestamp at_time,
      double modified_trend,
      double threshold,
      BandwidthUsage state);
  
  // Log delay-based BWE metrics
  void LogDelayBasedBweMetrics(
      Timestamp at_time,
      DataRate target_bitrate,
      BandwidthUsage delay_detector_state);
  
  // Log network controller metrics
  void LogNetworkControllerMetrics(
      Timestamp at_time,
      DataRate loss_based_target_rate,
      DataRate pushback_target_rate,
      DataRate stable_target_rate,
      uint8_t fraction_loss);

 private:
  GccMetricsLogger();
  ~GccMetricsLogger();
  
  // Write CSV header if file is new
  void WriteHeader() RTC_EXCLUSIVE_LOCKS_REQUIRED(mutex_);
  
  // Convert BandwidthUsage to string
  std::string BandwidthUsageToString(BandwidthUsage usage);
  
  Mutex mutex_;
  std::ofstream log_file_ RTC_GUARDED_BY(mutex_);
  bool is_initialized_ RTC_GUARDED_BY(mutex_);
  int peer_id_ RTC_GUARDED_BY(mutex_);
};

}  // namespace webrtc

#endif  // MODULES_CONGESTION_CONTROLLER_GOOG_CC_GCC_METRICS_LOGGER_H_