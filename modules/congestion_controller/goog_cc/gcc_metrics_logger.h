#ifndef MODULES_CONGESTION_CONTROLLER_GOOG_CC_GCC_METRICS_LOGGER_H_
#define MODULES_CONGESTION_CONTROLLER_GOOG_CC_GCC_METRICS_LOGGER_H_

#include <fstream>
#include <mutex>
#include <string>

#include "api/transport/bandwidth_usage.h"
#include "api/units/data_rate.h"
#include "api/units/timestamp.h"
#include "rtc_base/synchronization/mutex.h"

namespace webrtc {

// Singleton class that logs GCC metrics to a CSV file
class GccMetricsLogger {
 public:
  static GccMetricsLogger* GetInstance();
  
  // Initialize the logger with a file path
  void Initialize(const std::string& file_path);
  
  // Close the log file
  void Close();
  
  // Log trendline metrics
  void LogTrendlineMetrics(
      Timestamp at_time,
      double modified_trend,
      double threshold,
      BandwidthUsage state);
  
  // Log bandwidth estimate metrics
  void LogBweMetrics(
      Timestamp at_time,
      DataRate target_bitrate,
      DataRate actual_bitrate,
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
  void WriteHeader();
  
  static GccMetricsLogger* instance_;
  static webrtc::Mutex instance_lock_;
  
  std::ofstream log_file_;
  webrtc::Mutex file_lock_;
  bool is_initialized_;
};

}  // namespace webrtc

#endif  // MODULES_CONGESTION_CONTROLLER_GOOG_CC_GCC_METRICS_LOGGER_H_