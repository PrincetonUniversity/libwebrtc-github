/*
 *  Copyright 2012 The WebRTC Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef EXAMPLES_PEERCONNECTION_CLIENT_CONDUCTOR_H_
#define EXAMPLES_PEERCONNECTION_CLIENT_CONDUCTOR_H_

#include <deque>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include <atomic>
#include <fstream>
#include <mutex>
#include <thread>
#include "csv_writer.h" 

#include "api/media_stream_interface.h"
#include "api/peer_connection_interface.h"
#include "examples/peerconnection/client/main_wnd.h"
#include "examples/peerconnection/client/peer_connection_client.h"
#include "rtc_base/thread.h"
#include "modules/congestion_controller/goog_cc/gcc_metrics_logger.h"

namespace webrtc {
class VideoCaptureModule;
}  // namespace webrtc

namespace cricket {
class VideoRenderer;
}  // namespace cricket


class StatsCallback;  // Forward declaration

struct UIThreadCallbackData {
    int msg_id;
    void* data;

    UIThreadCallbackData(int id, void* d) : msg_id(id), data(d) {}
};


class Conductor : public webrtc::PeerConnectionObserver,
                  public webrtc::CreateSessionDescriptionObserver,
                  public PeerConnectionClientObserver,
                  public MainWndCallback {
 public:
  enum CallbackID {
    MEDIA_CHANNELS_INITIALIZED = 1,
    PEER_CONNECTION_CLOSED,
    SEND_MESSAGE_TO_PEER,
    NEW_TRACK_ADDED,
    TRACK_REMOVED,
  };

  Conductor(PeerConnectionClient* client, MainWindow* main_wnd, bool disable_gui, bool is_caller);

  void SetStunServer(const std::string& ip, int port);

  bool connection_active() const;

  void AutoLogin(const std::string& server, int port);

  void Close() override;

  void ProcessMessagesForNonGUIMode();

  void QueuePendingMessage(int msg_id, void* data) override;


 protected:
  ~Conductor();
  bool InitializePeerConnection();
  bool ReinitializePeerConnectionForLoopback();
  bool CreatePeerConnection();
  std::string GetStunServerString() const;  
  void DeletePeerConnection();
  void EnsureStreamingUI();
  void AddTracks();

  //
  // PeerConnectionObserver implementation.
  //

  void OnSignalingChange(
      webrtc::PeerConnectionInterface::SignalingState new_state) override {}
  void OnAddTrack(
      rtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver,
      const std::vector<rtc::scoped_refptr<webrtc::MediaStreamInterface>>&
          streams) override;
  void OnRemoveTrack(
      rtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver) override;
  void OnDataChannel(
      rtc::scoped_refptr<webrtc::DataChannelInterface> channel) override {}
  void OnRenegotiationNeeded() override {}
  void OnIceConnectionChange(
      webrtc::PeerConnectionInterface::IceConnectionState new_state) override {}
  void OnIceGatheringChange(
      webrtc::PeerConnectionInterface::IceGatheringState new_state) override {}
  void OnIceCandidate(const webrtc::IceCandidateInterface* candidate) override;
  void OnIceConnectionReceivingChange(bool receiving) override {}

  //
  // PeerConnectionClientObserver implementation.
  //

  void OnSignedIn() override;

  void OnDisconnected() override;

  void OnPeerConnected(int id, const std::string& name) override;

  void OnPeerDisconnected(int id) override;

  void OnMessageFromPeer(int peer_id, const std::string& message) override;

  void OnMessageFromPeerOnSignallingThread(int peer_id, const std::string& message);

  void OnMessageSent(int err) override;

  void OnServerConnectionFailure() override;

  //
  // MainWndCallback implementation.
  //

  void StartLogin(const std::string& server, int port) override;

  void DisconnectFromServer() override;

  void ConnectToPeer(int peer_id) override;

  void DisconnectFromCurrentPeer() override;

  void UIThreadCallback(int msg_id, void* data) override;

  // CreateSessionDescriptionObserver implementation.
  void OnSuccess(webrtc::SessionDescriptionInterface* desc) override;
  void OnFailure(webrtc::RTCError error) override;

 protected:
  // Send a message to the remote peer.
  void SendMessage(const std::string& json_object);

  // New methods for logging
  void StartLogging();
  void StopLogging();
  void LoggingThread();
  void LogMetrics();
  void OnStatsDelivered(
      const rtc::scoped_refptr<const webrtc::RTCStatsReport>& report);
  void LogLocalSDP(const std::string& type, const std::string& sdp);

  // New methods for gcc logging
  void InitializeGccLogging();
  void ShutdownGccLogging();

  int my_id_;
  int peer_id_; // the id of the remote peer
  bool loopback_; 
  std::unique_ptr<rtc::Thread> signaling_thread_;
  rtc::scoped_refptr<webrtc::PeerConnectionInterface> peer_connection_;
  rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface>
      peer_connection_factory_;
  PeerConnectionClient* client_;
  MainWindow* main_wnd_;
  std::deque<std::string*> pending_messages_;
  std::deque<UIThreadCallbackData*> pending_messages_NonGUI;
  std::string server_;

  // New member variables for logging
  std::atomic<bool> logging_active_;
  std::unique_ptr<std::thread> logging_thread_;
  std::mutex log_mutex_;
  unsigned long start_time_;
  std::unique_ptr<CSVWriter> pc_log_;
  std::unique_ptr<CSVWriter> in_rtp_log_;
  std::unique_ptr<CSVWriter> out_rtp_log_;
  std::unique_ptr<CSVWriter> video_src_log_;
  std::unique_ptr<std::ofstream> sdp_log_;

  // New member variables for gcc logging
  std::unique_ptr<std::ofstream> gcc_log_;

  bool disable_gui_;
  bool is_caller_;
  std::string stun_server_ip_;
  int stun_server_port_;  

  friend class StatsCallback;
};



// Inherits from "src/api/stats/rtc_stats_collector_callback.h" webrtc::RTCStatsCollectorCallback
// It acts as a bridge between the WebRTC stats collection and our Conductor class.
// When stats are collected, OnStatsDelivered is called with the report.
class StatsCallback : public webrtc::RTCStatsCollectorCallback {
 public:
  explicit StatsCallback(Conductor* conductor) : conductor_(conductor) {}
  void OnStatsDelivered(
      const rtc::scoped_refptr<const webrtc::RTCStatsReport>& report) override;

 private:
  Conductor* conductor_;
};




#endif  // EXAMPLES_PEERCONNECTION_CLIENT_CONDUCTOR_H_
