#include "csv_writer.h"

// Specialization for RTCIceCandidatePairStats
template<>
bool CSVWriter::hasValue(const webrtc::RTCIceCandidatePairStats& stats, const std::string& key) const {
    if (key == "transport_id") return stats.transport_id.has_value();
    if (key == "local_candidate_id") return stats.local_candidate_id.has_value();
    if (key == "remote_candidate_id") return stats.remote_candidate_id.has_value();
    if (key == "state") return stats.state.has_value();
    if (key == "priority") return stats.priority.has_value();
    if (key == "nominated") return stats.nominated.has_value();
    if (key == "writable") return stats.writable.has_value();
    if (key == "packets_sent") return stats.packets_sent.has_value();
    if (key == "packets_received") return stats.packets_received.has_value();
    if (key == "bytes_sent") return stats.bytes_sent.has_value();
    if (key == "bytes_received") return stats.bytes_received.has_value();
    if (key == "total_round_trip_time") return stats.total_round_trip_time.has_value();
    if (key == "current_round_trip_time") return stats.current_round_trip_time.has_value();
    if (key == "available_outgoing_bitrate") return stats.available_outgoing_bitrate.has_value();
    if (key == "available_incoming_bitrate") return stats.available_incoming_bitrate.has_value();
    if (key == "requests_received") return stats.requests_received.has_value();
    if (key == "requests_sent") return stats.requests_sent.has_value();
    if (key == "responses_received") return stats.responses_received.has_value();
    if (key == "responses_sent") return stats.responses_sent.has_value();
    if (key == "consent_requests_sent") return stats.consent_requests_sent.has_value();
    if (key == "packets_discarded_on_send") return stats.packets_discarded_on_send.has_value();
    if (key == "bytes_discarded_on_send") return stats.bytes_discarded_on_send.has_value();
    if (key == "last_packet_received_timestamp") return stats.last_packet_received_timestamp.has_value();
    if (key == "last_packet_sent_timestamp") return stats.last_packet_sent_timestamp.has_value();
    return false;
}

template<>
std::string CSVWriter::getValue(const webrtc::RTCIceCandidatePairStats& stats, const std::string& key) const {
    if (key == "transport_id") return stats.transport_id.value_or("");
    if (key == "local_candidate_id") return stats.local_candidate_id.value_or("");
    if (key == "remote_candidate_id") return stats.remote_candidate_id.value_or("");
    if (key == "state") return stats.state.value_or("");
    if (key == "priority") return std::to_string(stats.priority.value_or(0));
    if (key == "nominated") return stats.nominated.value_or(false) ? "true" : "false";
    if (key == "writable") return stats.writable.value_or(false) ? "true" : "false";
    if (key == "packets_sent") return std::to_string(stats.packets_sent.value_or(0));
    if (key == "packets_received") return std::to_string(stats.packets_received.value_or(0));
    if (key == "bytes_sent") return std::to_string(stats.bytes_sent.value_or(0));
    if (key == "bytes_received") return std::to_string(stats.bytes_received.value_or(0));
    if (key == "total_round_trip_time") return std::to_string(stats.total_round_trip_time.value_or(0.0));
    if (key == "current_round_trip_time") return std::to_string(stats.current_round_trip_time.value_or(0.0));
    if (key == "available_outgoing_bitrate") return std::to_string(stats.available_outgoing_bitrate.value_or(0.0));
    if (key == "available_incoming_bitrate") return std::to_string(stats.available_incoming_bitrate.value_or(0.0));
    if (key == "requests_received") return std::to_string(stats.requests_received.value_or(0));
    if (key == "requests_sent") return std::to_string(stats.requests_sent.value_or(0));
    if (key == "responses_received") return std::to_string(stats.responses_received.value_or(0));
    if (key == "responses_sent") return std::to_string(stats.responses_sent.value_or(0));
    if (key == "consent_requests_sent") return std::to_string(stats.consent_requests_sent.value_or(0));
    if (key == "packets_discarded_on_send") return std::to_string(stats.packets_discarded_on_send.value_or(0));
    if (key == "bytes_discarded_on_send") return std::to_string(stats.bytes_discarded_on_send.value_or(0));
    if (key == "last_packet_received_timestamp") return std::to_string(stats.last_packet_received_timestamp.value_or(0.0));
    if (key == "last_packet_sent_timestamp") return std::to_string(stats.last_packet_sent_timestamp.value_or(0.0));
    return "";
}

// Specialization for RTCInboundRtpStreamStats
template<>
bool CSVWriter::hasValue(const webrtc::RTCInboundRtpStreamStats& stats, const std::string& key) const {
    // RTCReceivedRtpStreamStats members
    if (key == "ssrc") return static_cast<const webrtc::RTCReceivedRtpStreamStats&>(stats).ssrc.has_value();
    if (key == "kind") return static_cast<const webrtc::RTCReceivedRtpStreamStats&>(stats).kind.has_value();
    if (key == "packets_lost") return static_cast<const webrtc::RTCReceivedRtpStreamStats&>(stats).packets_lost.has_value();
    if (key == "jitter") return static_cast<const webrtc::RTCReceivedRtpStreamStats&>(stats).jitter.has_value();

    // RTCInboundRtpStreamStats members
    if (key == "track_identifier") return stats.track_identifier.has_value();
    if (key == "mid") return stats.mid.has_value();
    if (key == "remote_id") return stats.remote_id.has_value();
    if (key == "packets_received") return stats.packets_received.has_value();    
    if (key == "packets_discarded") return stats.packets_discarded.has_value();
    if (key == "fec_packets_received") return stats.fec_packets_received.has_value();
    if (key == "fec_bytes_received") return stats.fec_bytes_received.has_value();
    if (key == "bytes_received") return stats.bytes_received.has_value();
    if (key == "header_bytes_received") return stats.header_bytes_received.has_value();
    if (key == "retransmitted_packets_received") return stats.retransmitted_packets_received.has_value();
    if (key == "retransmitted_bytes_received") return stats.retransmitted_bytes_received.has_value();
    if (key == "rtx_ssrc") return stats.rtx_ssrc.has_value();
    if (key == "jitter_buffer_delay") return stats.jitter_buffer_delay.has_value();
    if (key == "jitter_buffer_target_delay") return stats.jitter_buffer_target_delay.has_value();
    if (key == "jitter_buffer_minimum_delay") return stats.jitter_buffer_minimum_delay.has_value();
    if (key == "jitter_buffer_emitted_count") return stats.jitter_buffer_emitted_count.has_value();
    if (key == "total_samples_received") return stats.total_samples_received.has_value();
    if (key == "concealed_samples") return stats.concealed_samples.has_value();
    if (key == "silent_concealed_samples") return stats.silent_concealed_samples.has_value();
    if (key == "concealment_events") return stats.concealment_events.has_value();
    if (key == "inserted_samples_for_deceleration") return stats.inserted_samples_for_deceleration.has_value();
    if (key == "removed_samples_for_acceleration") return stats.removed_samples_for_acceleration.has_value();
    if (key == "audio_level") return stats.audio_level.has_value();
    if (key == "total_audio_energy") return stats.total_audio_energy.has_value();
    if (key == "total_samples_duration") return stats.total_samples_duration.has_value();
    if (key == "frames_received") return stats.frames_received.has_value();
    if (key == "frame_width") return stats.frame_width.has_value();
    if (key == "frame_height") return stats.frame_height.has_value();
    if (key == "frames_per_second") return stats.frames_per_second.has_value();
    if (key == "frames_decoded") return stats.frames_decoded.has_value();
    if (key == "key_frames_decoded") return stats.key_frames_decoded.has_value();
    if (key == "frames_dropped") return stats.frames_dropped.has_value();
    if (key == "total_decode_time") return stats.total_decode_time.has_value();
    if (key == "total_processing_delay") return stats.total_processing_delay.has_value();
    if (key == "total_assembly_time") return stats.total_assembly_time.has_value();
    if (key == "frames_assembled_from_multiple_packets") return stats.frames_assembled_from_multiple_packets.has_value();
    if (key == "total_inter_frame_delay") return stats.total_inter_frame_delay.has_value();
    if (key == "total_squared_inter_frame_delay") return stats.total_squared_inter_frame_delay.has_value();
    if (key == "pause_count") return stats.pause_count.has_value();
    if (key == "total_pauses_duration") return stats.total_pauses_duration.has_value();
    if (key == "freeze_count") return stats.freeze_count.has_value();
    if (key == "total_freezes_duration") return stats.total_freezes_duration.has_value();
    if (key == "content_type") return stats.content_type.has_value();
    if (key == "estimated_playout_timestamp") return stats.estimated_playout_timestamp.has_value();
    if (key == "decoder_implementation") return stats.decoder_implementation.has_value();
    if (key == "fir_count") return stats.fir_count.has_value();
    if (key == "pli_count") return stats.pli_count.has_value();
    if (key == "nack_count") return stats.nack_count.has_value();
    if (key == "qp_sum") return stats.qp_sum.has_value();
    if (key == "jitter_buffer_flushes") return stats.jitter_buffer_flushes.has_value();
    if (key == "delayed_packet_outage_samples") return stats.delayed_packet_outage_samples.has_value();
    if (key == "relative_packet_arrival_delay") return stats.relative_packet_arrival_delay.has_value();
    if (key == "interruption_count") return stats.interruption_count.has_value();
    if (key == "total_interruption_duration") return stats.total_interruption_duration.has_value();
    if (key == "min_playout_delay") return stats.min_playout_delay.has_value();
    return false;
}

template<>
std::string CSVWriter::getValue(const webrtc::RTCInboundRtpStreamStats& stats, const std::string& key) const {
    // RTCReceivedRtpStreamStats members
    if (key == "ssrc") return std::to_string(static_cast<const webrtc::RTCReceivedRtpStreamStats&>(stats).ssrc.value_or(0));
    if (key == "kind") return static_cast<const webrtc::RTCReceivedRtpStreamStats&>(stats).kind.value_or("");
    if (key == "packets_lost") return std::to_string(static_cast<const webrtc::RTCReceivedRtpStreamStats&>(stats).packets_lost.value_or(0));
    if (key == "jitter") return std::to_string(static_cast<const webrtc::RTCReceivedRtpStreamStats&>(stats).jitter.value_or(0.0));

    // RTCInboundRtpStreamStats members
    if (key == "track_identifier") return stats.track_identifier.value_or("");
    if (key == "mid") return stats.mid.value_or("");
    if (key == "remote_id") return stats.remote_id.value_or("");
    if (key == "packets_received") return std::to_string(stats.packets_received.value_or(0));    
    if (key == "packets_discarded") return std::to_string(stats.packets_discarded.value_or(0));
    if (key == "fec_packets_received") return std::to_string(stats.fec_packets_received.value_or(0));
    if (key == "fec_bytes_received") return std::to_string(stats.fec_bytes_received.value_or(0));
    if (key == "bytes_received") return std::to_string(stats.bytes_received.value_or(0));
    if (key == "header_bytes_received") return std::to_string(stats.header_bytes_received.value_or(0));
    if (key == "retransmitted_packets_received") return std::to_string(stats.retransmitted_packets_received.value_or(0));
    if (key == "retransmitted_bytes_received") return std::to_string(stats.retransmitted_bytes_received.value_or(0));
    if (key == "rtx_ssrc") return std::to_string(stats.rtx_ssrc.value_or(0));
    if (key == "jitter_buffer_delay") return std::to_string(stats.jitter_buffer_delay.value_or(0.0));
    if (key == "jitter_buffer_target_delay") return std::to_string(stats.jitter_buffer_target_delay.value_or(0.0));
    if (key == "jitter_buffer_minimum_delay") return std::to_string(stats.jitter_buffer_minimum_delay.value_or(0.0));
    if (key == "jitter_buffer_emitted_count") return std::to_string(stats.jitter_buffer_emitted_count.value_or(0));
    if (key == "total_samples_received") return std::to_string(stats.total_samples_received.value_or(0));
    if (key == "concealed_samples") return std::to_string(stats.concealed_samples.value_or(0));
    if (key == "silent_concealed_samples") return std::to_string(stats.silent_concealed_samples.value_or(0));
    if (key == "concealment_events") return std::to_string(stats.concealment_events.value_or(0));
    if (key == "inserted_samples_for_deceleration") return std::to_string(stats.inserted_samples_for_deceleration.value_or(0));
    if (key == "removed_samples_for_acceleration") return std::to_string(stats.removed_samples_for_acceleration.value_or(0));
    if (key == "audio_level") return std::to_string(stats.audio_level.value_or(0.0));
    if (key == "total_audio_energy") return std::to_string(stats.total_audio_energy.value_or(0.0));
    if (key == "total_samples_duration") return std::to_string(stats.total_samples_duration.value_or(0.0));
    if (key == "frames_received") return std::to_string(stats.frames_received.value_or(0));
    if (key == "frame_width") return std::to_string(stats.frame_width.value_or(0));
    if (key == "frame_height") return std::to_string(stats.frame_height.value_or(0));
    if (key == "frames_per_second") return std::to_string(stats.frames_per_second.value_or(0.0));
    if (key == "frames_decoded") return std::to_string(stats.frames_decoded.value_or(0));
    if (key == "key_frames_decoded") return std::to_string(stats.key_frames_decoded.value_or(0));
    if (key == "frames_dropped") return std::to_string(stats.frames_dropped.value_or(0));
    if (key == "total_decode_time") return std::to_string(stats.total_decode_time.value_or(0.0));
    if (key == "total_processing_delay") return std::to_string(stats.total_processing_delay.value_or(0.0));
    if (key == "total_assembly_time") return std::to_string(stats.total_assembly_time.value_or(0.0));
    if (key == "frames_assembled_from_multiple_packets") return std::to_string(stats.frames_assembled_from_multiple_packets.value_or(0));
    if (key == "total_inter_frame_delay") return std::to_string(stats.total_inter_frame_delay.value_or(0.0));
    if (key == "total_squared_inter_frame_delay") return std::to_string(stats.total_squared_inter_frame_delay.value_or(0.0));
    if (key == "pause_count") return std::to_string(stats.pause_count.value_or(0));
    if (key == "total_pauses_duration") return std::to_string(stats.total_pauses_duration.value_or(0.0));
    if (key == "freeze_count") return std::to_string(stats.freeze_count.value_or(0));
    if (key == "total_freezes_duration") return std::to_string(stats.total_freezes_duration.value_or(0.0));
    if (key == "content_type") return stats.content_type.value_or("");
    if (key == "estimated_playout_timestamp") return std::to_string(stats.estimated_playout_timestamp.value_or(0.0));
    if (key == "decoder_implementation") return stats.decoder_implementation.value_or("");
    if (key == "fir_count") return std::to_string(stats.fir_count.value_or(0));
    if (key == "pli_count") return std::to_string(stats.pli_count.value_or(0));
    if (key == "nack_count") return std::to_string(stats.nack_count.value_or(0));
    if (key == "qp_sum") return std::to_string(stats.qp_sum.value_or(0));
    if (key == "jitter_buffer_flushes") return std::to_string(stats.jitter_buffer_flushes.value_or(0));
    if (key == "delayed_packet_outage_samples") return std::to_string(stats.delayed_packet_outage_samples.value_or(0));
    if (key == "relative_packet_arrival_delay") return std::to_string(stats.relative_packet_arrival_delay.value_or(0.0));
    if (key == "interruption_count") return std::to_string(stats.interruption_count.value_or(0));
    if (key == "total_interruption_duration") return std::to_string(stats.total_interruption_duration.value_or(0.0));
    if (key == "min_playout_delay") return std::to_string(stats.min_playout_delay.value_or(0.0));
    return "";
}

// Specialization for RTCOutboundRtpStreamStats
template<>
bool CSVWriter::hasValue(const webrtc::RTCOutboundRtpStreamStats& stats, const std::string& key) const {
    // RTCSentRtpStreamStats members
    if (key == "ssrc") return static_cast<const webrtc::RTCSentRtpStreamStats&>(stats).ssrc.has_value();
    if (key == "kind") return static_cast<const webrtc::RTCSentRtpStreamStats&>(stats).kind.has_value();
    if (key == "packets_sent") return static_cast<const webrtc::RTCSentRtpStreamStats&>(stats).packets_sent.has_value();
    if (key == "bytes_sent") return static_cast<const webrtc::RTCSentRtpStreamStats&>(stats).bytes_sent.has_value();

    // RTCOutboundRtpStreamStats members
    if (key == "media_source_id") return stats.media_source_id.has_value();
    if (key == "remote_id") return stats.remote_id.has_value();
    if (key == "mid") return stats.mid.has_value();
    if (key == "rid") return stats.rid.has_value();
    if (key == "retransmitted_packets_sent") return stats.retransmitted_packets_sent.has_value();
    if (key == "header_bytes_sent") return stats.header_bytes_sent.has_value();
    if (key == "retransmitted_bytes_sent") return stats.retransmitted_bytes_sent.has_value();
    if (key == "target_bitrate") return stats.target_bitrate.has_value();
    if (key == "frames_encoded") return stats.frames_encoded.has_value();
    if (key == "key_frames_encoded") return stats.key_frames_encoded.has_value();
    if (key == "total_encode_time") return stats.total_encode_time.has_value();
    if (key == "total_encoded_bytes_target") return stats.total_encoded_bytes_target.has_value();
    if (key == "frame_width") return stats.frame_width.has_value();
    if (key == "frame_height") return stats.frame_height.has_value();
    if (key == "frames_per_second") return stats.frames_per_second.has_value();
    if (key == "frames_sent") return stats.frames_sent.has_value();
    if (key == "huge_frames_sent") return stats.huge_frames_sent.has_value();
    if (key == "total_packet_send_delay") return stats.total_packet_send_delay.has_value();
    if (key == "quality_limitation_reason") return stats.quality_limitation_reason.has_value();
    if (key == "quality_limitation_durations") return stats.quality_limitation_durations.has_value();
    if (key == "quality_limitation_resolution_changes") return stats.quality_limitation_resolution_changes.has_value();
    if (key == "content_type") return stats.content_type.has_value();
    if (key == "encoder_implementation") return stats.encoder_implementation.has_value();
    if (key == "fir_count") return stats.fir_count.has_value();
    if (key == "pli_count") return stats.pli_count.has_value();
    if (key == "nack_count") return stats.nack_count.has_value();
    if (key == "qp_sum") return stats.qp_sum.has_value();
    if (key == "active") return stats.active.has_value();
    if (key == "power_efficient_encoder") return stats.power_efficient_encoder.has_value();
    if (key == "scalability_mode") return stats.scalability_mode.has_value();
    if (key == "rtx_ssrc") return stats.rtx_ssrc.has_value();
    return false;
}

template<>
std::string CSVWriter::getValue(const webrtc::RTCOutboundRtpStreamStats& stats, const std::string& key) const {
    // RTCSentRtpStreamStats members
    if (key == "ssrc") return std::to_string(static_cast<const webrtc::RTCSentRtpStreamStats&>(stats).ssrc.value_or(0));
    if (key == "kind") return static_cast<const webrtc::RTCSentRtpStreamStats&>(stats).kind.value_or("");
    if (key == "packets_sent") return std::to_string(static_cast<const webrtc::RTCSentRtpStreamStats&>(stats).packets_sent.value_or(0));
    if (key == "bytes_sent") return std::to_string(static_cast<const webrtc::RTCSentRtpStreamStats&>(stats).bytes_sent.value_or(0));

    // RTCOutboundRtpStreamStats members
    if (key == "media_source_id") return stats.media_source_id.value_or("");
    if (key == "remote_id") return stats.remote_id.value_or("");
    if (key == "mid") return stats.mid.value_or("");
    if (key == "rid") return stats.rid.value_or("");
    if (key == "retransmitted_packets_sent") return std::to_string(stats.retransmitted_packets_sent.value_or(0));
    if (key == "header_bytes_sent") return std::to_string(stats.header_bytes_sent.value_or(0));
    if (key == "retransmitted_bytes_sent") return std::to_string(stats.retransmitted_bytes_sent.value_or(0));
    if (key == "target_bitrate") return std::to_string(stats.target_bitrate.value_or(0.0));
    if (key == "frames_encoded") return std::to_string(stats.frames_encoded.value_or(0));
    if (key == "key_frames_encoded") return std::to_string(stats.key_frames_encoded.value_or(0));
    if (key == "total_encode_time") return std::to_string(stats.total_encode_time.value_or(0.0));
    if (key == "total_encoded_bytes_target") return std::to_string(stats.total_encoded_bytes_target.value_or(0));
    if (key == "frame_width") return std::to_string(stats.frame_width.value_or(0));
    if (key == "frame_height") return std::to_string(stats.frame_height.value_or(0));
    if (key == "frames_per_second") return std::to_string(stats.frames_per_second.value_or(0.0));
    if (key == "frames_sent") return std::to_string(stats.frames_sent.value_or(0));
    if (key == "huge_frames_sent") return std::to_string(stats.huge_frames_sent.value_or(0));
    if (key == "total_packet_send_delay") return std::to_string(stats.total_packet_send_delay.value_or(0.0));
    if (key == "quality_limitation_reason") return stats.quality_limitation_reason.value_or("");
    if (key == "quality_limitation_durations") {
        if (stats.quality_limitation_durations.has_value()) {
            const auto& durations = *stats.quality_limitation_durations;
            std::stringstream ss;
            ss << durations.find("bandwidth")->second << ","
               << durations.find("cpu")->second << ","
               << durations.find("none")->second << ","
               << durations.find("other")->second;
            return ss.str();
        }
        return ",,,,";
    }
    if (key == "quality_limitation_resolution_changes") return std::to_string(stats.quality_limitation_resolution_changes.value_or(0));
    if (key == "content_type") return stats.content_type.value_or("");
    if (key == "encoder_implementation") return stats.encoder_implementation.value_or("");
    if (key == "fir_count") return std::to_string(stats.fir_count.value_or(0));
    if (key == "pli_count") return std::to_string(stats.pli_count.value_or(0));
    if (key == "nack_count") return std::to_string(stats.nack_count.value_or(0));
    if (key == "qp_sum") return std::to_string(stats.qp_sum.value_or(0));
    if (key == "active") return stats.active.value_or(false) ? "true" : "false";
    if (key == "power_efficient_encoder") return stats.power_efficient_encoder.value_or(false) ? "true" : "false";
    if (key == "scalability_mode") return stats.scalability_mode.value_or("");
    if (key == "rtx_ssrc") return std::to_string(stats.rtx_ssrc.value_or(0));
    return "";
}