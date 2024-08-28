#ifndef CSV_WRITER_H_
#define CSV_WRITER_H_

#include <fstream>
#include <string>
#include <sstream>
#include "absl/types/optional.h"
#include "api/stats/rtc_stats_report.h"
#include "api/stats/rtcstats_objects.h"

class CSVWriter {
public:
    CSVWriter(const std::string& filename) : file_(filename) {}

    ~CSVWriter() {
        if (file_.is_open()) {
            file_.close();
        }
    }

    template<typename T>
    CSVWriter& operator<<(const T& val) {
        file_ << val;
        return *this;
    }

    CSVWriter& operator<<(std::ostream& (*manip)(std::ostream&)) {
        manip(file_);
        return *this;
    }

    template<typename StatsType>
    void writeFromDict(const StatsType& stats, const std::string& key, const std::string& separator = ",") {
        if (hasValue(stats, key)) {
            file_ << getValue(stats, key) << separator;
        } else {
            file_ << separator;
        }
    }

    void flush() {
        file_.flush();
    }

private:
    std::ofstream file_;

    template<typename StatsType>
    bool hasValue(const StatsType& stats, const std::string& key) const;

    template<typename StatsType>
    std::string getValue(const StatsType& stats, const std::string& key) const;
};

// Declare specializations
template<>
bool CSVWriter::hasValue(const webrtc::RTCIceCandidatePairStats& stats, const std::string& key) const;

template<>
std::string CSVWriter::getValue(const webrtc::RTCIceCandidatePairStats& stats, const std::string& key) const;

template<>
bool CSVWriter::hasValue(const webrtc::RTCInboundRtpStreamStats& stats, const std::string& key) const;

template<>
std::string CSVWriter::getValue(const webrtc::RTCInboundRtpStreamStats& stats, const std::string& key) const;

template<>
bool CSVWriter::hasValue(const webrtc::RTCOutboundRtpStreamStats& stats, const std::string& key) const;

template<>
std::string CSVWriter::getValue(const webrtc::RTCOutboundRtpStreamStats& stats, const std::string& key) const;

#endif  // CSV_WRITER_H_