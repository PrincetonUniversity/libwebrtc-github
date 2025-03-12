#include "modules/congestion_controller/goog_cc/gcc_metrics_logger.h"

#include <iomanip>
#include "rtc_base/checks.h"
#include "rtc_base/logging.h"

namespace webrtc {

// Use WebRTC's thread-local storage pattern for singletons
GccMetricsLogger* GccMetricsLogger::GetInstance() {
  // This pattern avoids the need for static members with exit-time destructors
  static GccMetricsLogger* const instance = new GccMetricsLogger();
  return instance;
}

GccMetricsLogger::GccMetricsLogger() 
    : is_initialized_(false), peer_id_(-1) {}

GccMetricsLogger::~GccMetricsLogger() {
  Close();
}

void GccMetricsLogger::Initialize(const std::string& file_path) {
  MutexLock lock(&mutex_);
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
  // WriteHeader is called with mutex_ held
  WriteHeader();
  RTC_LOG(LS_INFO) << "GCC metrics logging initialized: " << file_path;
}

void GccMetricsLogger::Close() {
  MutexLock lock(&mutex_);
  if (is_initialized_ && log_file_.is_open()) {
    log_file_.close();
    is_initialized_ = false;
    RTC_LOG(LS_INFO) << "GCC metrics logging closed";
  }
}

void GccMetricsLogger::SetPeerId(int peer_id) {
  MutexLock lock(&mutex_);
  peer_id_ = peer_id;
}

void GccMetricsLogger::WriteHeader() {
  log_file_ << "timestamp_ms,component,peer_id,modified_trend,threshold,bandwidth_state,"
            << "target_bitrate_bps,loss_based_target_rate_bps,"
            << "pushback_target_rate_bps,stable_target_rate_bps,fraction_loss"
            << std::endl;
}

std::string GccMetricsLogger::BandwidthUsageToString(BandwidthUsage usage) {
  switch (usage) {
    case BandwidthUsage::kBwNormal:
      return "normal";
    case BandwidthUsage::kBwOverusing:
      return "overusing";
    case BandwidthUsage::kBwUnderusing:
      return "underusing";
    default:
      return "unknown";
  }
}

void GccMetricsLogger::LogTrendlineMetrics(
    Timestamp at_time,
    double modified_trend,
    double threshold,
    BandwidthUsage state) {
  MutexLock lock(&mutex_);
  if (!is_initialized_ || !log_file_.is_open()) {
    return;
  }
  
  log_file_ << at_time.ms() << ","
            << "trendline," 
            << peer_id_ << ","
            << modified_trend << ","
            << threshold << ","
            << BandwidthUsageToString(state) << ","
            << ",,,,,"  // Empty fields for other metrics
            << std::endl;
}

void GccMetricsLogger::LogDelayBasedBweMetrics(
    Timestamp at_time,
    DataRate target_bitrate,
    BandwidthUsage delay_detector_state) {
  MutexLock lock(&mutex_);
  if (!is_initialized_ || !log_file_.is_open()) {
    return;
  }
  
  log_file_ << at_time.ms() << ","
            << "delay_bwe,"
            << peer_id_ << ","
            << ",,"  // Empty fields for trendline metrics
            << BandwidthUsageToString(delay_detector_state) << ","
            << target_bitrate.bps() << ","
            << ",,,,"  // Empty fields for network controller metrics
            << std::endl;
}

void GccMetricsLogger::LogNetworkControllerMetrics(
    Timestamp at_time,
    DataRate loss_based_target_rate,
    DataRate pushback_target_rate,
    DataRate stable_target_rate,
    uint8_t fraction_loss) {
  MutexLock lock(&mutex_);
  if (!is_initialized_ || !log_file_.is_open()) {
    return;
  }
  
  log_file_ << at_time.ms() << ","
            << "network_controller,"
            << peer_id_ << ","
            << ",,,"  // Empty fields for other metrics
            << ","  // Empty field for target_bitrate_bps
            << loss_based_target_rate.bps() << ","
            << pushback_target_rate.bps() << ","
            << stable_target_rate.bps() << ","
            << static_cast<int>(fraction_loss)
            << std::endl;
}

}  // namespace webrtc