#include "modules/congestion_controller/goog_cc/gcc_metrics_logger.h"

#include <iomanip>
#include "rtc_base/logging.h"

namespace webrtc {

GccMetricsLogger* GccMetricsLogger::instance_ = nullptr;
webrtc::Mutex GccMetricsLogger::instance_lock_;

GccMetricsLogger* GccMetricsLogger::GetInstance() {
  webrtc::MutexLock lock(&instance_lock_);
  if (instance_ == nullptr) {
    instance_ = new GccMetricsLogger();
  }
  return instance_;
}

GccMetricsLogger::GccMetricsLogger() : is_initialized_(false) {}

GccMetricsLogger::~GccMetricsLogger() {
  Close();
}

void GccMetricsLogger::Initialize(const std::string& file_path) {
  webrtc::MutexLock lock(&file_lock_);
  if (is_initialized_) {
    RTC_LOG(LS_WARNING) << "GccMetricsLogger already initialized";
    return;
  }
  
  log_file_.open(file_path, std::ios::out);
  if (!log_file_.is_open()) {
    RTC_LOG(LS_ERROR) << "Failed to open GCC metrics log file: " << file_path;
    return;
  }
  
  is_initialized_ = true;
  WriteHeader();
  RTC_LOG(LS_INFO) << "GCC metrics logging initialized: " << file_path;
}

void GccMetricsLogger::Close() {
  webrtc::MutexLock lock(&file_lock_);
  if (is_initialized_ && log_file_.is_open()) {
    log_file_.close();
    is_initialized_ = false;
    RTC_LOG(LS_INFO) << "GCC metrics logging closed";
  }
}

void GccMetricsLogger::WriteHeader() {
  log_file_ << "timestamp_ms,"
            << "component,"
            << "modified_trend,"
            << "threshold,"
            << "bandwidth_state,"
            << "target_bitrate_bps,"
            << "actual_bitrate_bps,"
            << "pushback_target_rate_bps,"
            << "stable_target_rate_bps,"
            << "fraction_loss"
            << std::endl;
}

void GccMetricsLogger::LogTrendlineMetrics(
    Timestamp at_time,
    double modified_trend,
    double threshold,
    BandwidthUsage state) {
  webrtc::MutexLock lock(&file_lock_);
  if (!is_initialized_ || !log_file_.is_open()) {
    return;
  }
  
  std::string state_str;
  switch (state) {
    case BandwidthUsage::kBwNormal:
      state_str = "normal";
      break;
    case BandwidthUsage::kBwOverusing:
      state_str = "overusing";
      break;
    case BandwidthUsage::kBwUnderusing:
      state_str = "underusing";
      break;
    default:
      state_str = "unknown";
  }
  
  log_file_ << at_time.ms() << ","
            << "trendline,"
            << modified_trend << ","
            << threshold << ","
            << state_str << ","
            << ",,,," // Empty fields for other metrics
            << std::endl;
}

void GccMetricsLogger::LogBweMetrics(
    Timestamp at_time,
    DataRate target_bitrate,
    DataRate actual_bitrate,
    BandwidthUsage delay_detector_state) {
  webrtc::MutexLock lock(&file_lock_);
  if (!is_initialized_ || !log_file_.is_open()) {
    return;
  }
  
  std::string state_str;
  switch (delay_detector_state) {
    case BandwidthUsage::kBwNormal:
      state_str = "normal";
      break;
    case BandwidthUsage::kBwOverusing:
      state_str = "overusing";
      break;
    case BandwidthUsage::kBwUnderusing:
      state_str = "underusing";
      break;
    default:
      state_str = "unknown";
  }
  
  log_file_ << at_time.ms() << ","
            << "delay_bwe,"
            << ",,"  // Empty fields for trendline metrics
            << state_str << ","
            << target_bitrate.bps() << ","
            << actual_bitrate.bps() << ","
            << ",," // Empty fields for network controller metrics
            << std::endl;
}

void GccMetricsLogger::LogNetworkControllerMetrics(
    Timestamp at_time,
    DataRate loss_based_target_rate,
    DataRate pushback_target_rate,
    DataRate stable_target_rate,
    uint8_t fraction_loss) {
  webrtc::MutexLock lock(&file_lock_);
  if (!is_initialized_ || !log_file_.is_open()) {
    return;
  }
  
  log_file_ << at_time.ms() << ","
            << "network_controller,"
            << ",,,"  // Empty fields for other metrics
            << loss_based_target_rate.bps() << ","
            << ","  // Empty field for actual_bitrate_bps
            << pushback_target_rate.bps() << ","
            << stable_target_rate.bps() << ","
            << static_cast<int>(fraction_loss)
            << std::endl;
}

}  // namespace webrtc