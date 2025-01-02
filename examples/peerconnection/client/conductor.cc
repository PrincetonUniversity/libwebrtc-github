/*
 *  Copyright 2012 The WebRTC Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "examples/peerconnection/client/conductor.h"

#include <stddef.h>
#include <stdint.h>

#include <memory>
#include <utility>
#include <vector>

#include "absl/memory/memory.h"
#include "absl/types/optional.h"
#include "api/audio/audio_device.h"
#include "api/audio/audio_mixer.h"
#include "api/audio/audio_processing.h"
#include "api/audio_codecs/audio_decoder_factory.h"
#include "api/audio_codecs/audio_encoder_factory.h"
#include "api/audio_codecs/builtin_audio_decoder_factory.h"
#include "api/audio_codecs/builtin_audio_encoder_factory.h"
#include "api/audio_options.h"
#include "api/create_peerconnection_factory.h"
#include "api/rtp_sender_interface.h"
#include "api/video_codecs/sdp_video_format.h"
#include "api/video_codecs/scalability_mode.h"
#include "api/video_codecs/video_decoder_factory.h"
#include "api/video_codecs/video_decoder_factory_template.h"
#include "api/video_codecs/video_decoder_factory_template_dav1d_adapter.h"
#include "api/video_codecs/video_decoder_factory_template_libvpx_vp8_adapter.h"
#include "api/video_codecs/video_decoder_factory_template_libvpx_vp9_adapter.h"
#include "api/video_codecs/video_decoder_factory_template_open_h264_adapter.h"
#include "api/video_codecs/video_encoder_factory.h"
#include "api/video_codecs/video_encoder_factory_template.h"
#include "api/video_codecs/video_encoder_factory_template_libaom_av1_adapter.h"
#include "api/video_codecs/video_encoder_factory_template_libvpx_vp8_adapter.h"
#include "api/video_codecs/video_encoder_factory_template_libvpx_vp9_adapter.h"
#include "api/video_codecs/video_encoder_factory_template_open_h264_adapter.h"
#include "examples/peerconnection/client/defaults.h"
#include "modules/video_capture/video_capture.h"
#include "modules/video_capture/video_capture_factory.h"
#include "p2p/base/port_allocator.h"
#include "pc/video_track_source.h"
#include "rtc_base/checks.h"
#include "rtc_base/logging.h"
#include "rtc_base/rtc_certificate_generator.h"
#include "rtc_base/strings/json.h"
#include "test/vcm_capturer.h"

#include <chrono>
#include "examples/peerconnection/client/csv_writer.h"
#include "api/stats/rtc_stats_report.h"
#include "api/stats/rtcstats_objects.h"

// 
// class CustomVideoEncoderFactory : public webrtc::VideoEncoderFactory {
//  public:
//   CustomVideoEncoderFactory() {
//     internal_factory_ = std::make_unique<webrtc::VideoEncoderFactoryTemplate<
//         webrtc::LibaomAv1EncoderTemplateAdapter>>();
    
//     // Log initial supported formats
//     RTC_LOG(LS_INFO) << "Initial supported formats:";
//     auto initial_formats = internal_factory_->GetSupportedFormats();
//     for (const auto& format : initial_formats) {
//       RTC_LOG(LS_INFO) << "Codec: " << format.name << " Scalability modes:";
//       for (const auto& mode : format.scalability_modes) {
//         RTC_LOG(LS_INFO) << "  " << ScalabilityModeToString(mode);
//       }
//     }
//   }

//   std::vector<webrtc::SdpVideoFormat> GetSupportedFormats() const override {
//     std::vector<webrtc::SdpVideoFormat> formats;
//     auto original_formats = internal_factory_->GetSupportedFormats();
    
//     for (auto format : original_formats) {
//       if (format.name == "AV1") {
//         // Clear any existing scalability modes and only set L1T2
//         format.scalability_modes.clear();
//         format.scalability_modes = {webrtc::ScalabilityMode::kL1T2};
        
//         // Add L1T2 to parameters explicitly
//         format.parameters["scalability-mode"] = "L1T2";
        
//         formats.push_back(format);
        
//         RTC_LOG(LS_INFO) << "Offering AV1 with forced L1T2 scalability mode";
//       }
//     }
//     return formats;
//   }

//   webrtc::VideoEncoderFactory::CodecSupport QueryCodecSupport(
//       const webrtc::SdpVideoFormat& format,
//       absl::optional<std::string> scalability_mode) const override {
//     // Only support L1T2 for AV1
//     if (format.name == "AV1") {
//       if (!scalability_mode.has_value() || scalability_mode.value() == "L1T2") {
//         return {.is_supported = true};
//       }
//       return {.is_supported = false};
//     }
//     return internal_factory_->QueryCodecSupport(format, scalability_mode);
//   }

//   std::unique_ptr<webrtc::VideoEncoder> Create(
//       const webrtc::Environment& env,
//       const webrtc::SdpVideoFormat& format) override {
//     if (format.name == "AV1") {
//       // Ensure format has L1T2 parameter
//       auto modified_format = format;
//       modified_format.parameters["scalability-mode"] = "L1T2";
//       return internal_factory_->Create(env, modified_format);
//     }
//     return internal_factory_->Create(env, format);
//   }

//  private:
//   std::unique_ptr<webrtc::VideoEncoderFactory> internal_factory_;
// };


namespace {
// Names used for a IceCandidate JSON object.
const char kCandidateSdpMidName[] = "sdpMid";
const char kCandidateSdpMlineIndexName[] = "sdpMLineIndex";
const char kCandidateSdpName[] = "candidate";

// Names used for a SessionDescription JSON object.
const char kSessionDescriptionTypeName[] = "type";
const char kSessionDescriptionSdpName[] = "sdp";

// Inherited from webrtc::SetSessionDescriptionObserver, override OnSuccess and OnFailure
class DummySetSessionDescriptionObserver
    : public webrtc::SetSessionDescriptionObserver {
 public:
  static rtc::scoped_refptr<DummySetSessionDescriptionObserver> Create() {
    return rtc::make_ref_counted<DummySetSessionDescriptionObserver>();
  }
  virtual void OnSuccess() { RTC_LOG(LS_INFO) << __FUNCTION__; }
  virtual void OnFailure(webrtc::RTCError error) {
    RTC_LOG(LS_INFO) << __FUNCTION__ << " " << ToString(error.type()) << ": "
                     << error.message();
  }
};

class CapturerTrackSource : public webrtc::VideoTrackSource {
 public:
  static rtc::scoped_refptr<CapturerTrackSource> Create() {
    const size_t kWidth = 640;
    const size_t kHeight = 480;
    const size_t kFps = 30;
    std::unique_ptr<webrtc::test::VcmCapturer> capturer;
    std::unique_ptr<webrtc::VideoCaptureModule::DeviceInfo> info(
        webrtc::VideoCaptureFactory::CreateDeviceInfo());
    if (!info) {
      return nullptr;
    }
    int num_devices = info->NumberOfDevices();
    for (int i = 0; i < num_devices; ++i) {
      capturer = absl::WrapUnique(
          webrtc::test::VcmCapturer::Create(kWidth, kHeight, kFps, i));
      if (capturer) {
        return rtc::make_ref_counted<CapturerTrackSource>(std::move(capturer));
      }
    }

    return nullptr;
  }

 protected:
  explicit CapturerTrackSource(
      std::unique_ptr<webrtc::test::VcmCapturer> capturer)
      : VideoTrackSource(/*remote=*/false), capturer_(std::move(capturer)) {}

 private:
  rtc::VideoSourceInterface<webrtc::VideoFrame>* source() override {
    return capturer_.get();
  }
  std::unique_ptr<webrtc::test::VcmCapturer> capturer_;
};

}  // namespace_id



void StatsCallback::OnStatsDelivered(
    const rtc::scoped_refptr<const webrtc::RTCStatsReport>& report) {
  conductor_->OnStatsDelivered(report);
}


Conductor::Conductor(PeerConnectionClient* client, MainWindow* main_wnd, bool disable_gui, bool is_caller)
    : my_id_(-1),
      peer_id_(-1),
      loopback_(false),
      client_(client),
      main_wnd_(main_wnd),
      logging_active_(false),
      disable_gui_(disable_gui),
      is_caller_(is_caller),
      stun_server_ip_(""),
      stun_server_port_(0) {
  client_->RegisterObserver(this);
  main_wnd->RegisterObserver(this);
}

Conductor::~Conductor() {
  RTC_DCHECK(!peer_connection_);
  StopLogging();
  if (sdp_log_) {
      sdp_log_.reset();  // Close the SDP log file
  }  
}

void Conductor::SetStunServer(const std::string& ip, int port) {
  stun_server_ip_ = ip;
  stun_server_port_ = port;
}

std::string Conductor::GetStunServerString() const {
  if (!stun_server_ip_.empty() && stun_server_port_ > 0) {
    return "stun:" + stun_server_ip_ + ":" + std::to_string(stun_server_port_);
  }
  return GetEnvVarOrDefault("WEBRTC_CONNECT", "stun:stun.l.google.com:19302");
}

bool Conductor::connection_active() const {
  return peer_connection_ != nullptr;
}

// To close the local program, first send a sign out signal to the signaling server, 
// then destroy the PeerConnection object.
void Conductor::Close() {
  client_->SignOut(); // send sign out signal to signaling server
  DeletePeerConnection();
}

bool Conductor::InitializePeerConnection() {
  RTC_LOG(LS_INFO) << __FUNCTION__ << ": Initializing peer connection";

  RTC_DCHECK(!peer_connection_factory_);
  RTC_DCHECK(!peer_connection_);

  if (!signaling_thread_.get()) {
    RTC_LOG(LS_INFO) << "Creating signaling thread";
    signaling_thread_ = rtc::Thread::CreateWithSocketServer();
    signaling_thread_->Start();
  }

  // 1. create PeerConnectionFactory with audio and video codecs
  RTC_LOG(LS_INFO) << "1. Creating PeerConnectionFactory";
  // peer_connection_factory_ = webrtc::CreatePeerConnectionFactory(
  //     nullptr /* network_thread */, nullptr /* worker_thread */,
  //     signaling_thread_.get(), nullptr /* default_adm */,
  //     webrtc::CreateBuiltinAudioEncoderFactory(),
  //     webrtc::CreateBuiltinAudioDecoderFactory(),
  //     std::make_unique<webrtc::VideoEncoderFactoryTemplate<
  //         webrtc::LibaomAv1EncoderTemplateAdapter,
  //         webrtc::LibvpxVp8EncoderTemplateAdapter,
  //         webrtc::LibvpxVp9EncoderTemplateAdapter,
  //         webrtc::OpenH264EncoderTemplateAdapter>>(),
  //     std::make_unique<webrtc::VideoDecoderFactoryTemplate<
  //         webrtc::Dav1dDecoderTemplateAdapter,
  //         webrtc::LibvpxVp8DecoderTemplateAdapter,
  //         webrtc::LibvpxVp9DecoderTemplateAdapter,
  //         webrtc::OpenH264DecoderTemplateAdapter>>(),
  //     nullptr /* audio_mixer */, nullptr /* audio_processing */);
  peer_connection_factory_ = webrtc::CreatePeerConnectionFactory(
      nullptr /* network_thread */, nullptr /* worker_thread */,
      signaling_thread_.get(), nullptr /* default_adm */,
      webrtc::CreateBuiltinAudioEncoderFactory(),
      webrtc::CreateBuiltinAudioDecoderFactory(),
      std::make_unique<webrtc::VideoEncoderFactoryTemplate<
          webrtc::LibaomAv1EncoderTemplateAdapter>>(),
      std::make_unique<webrtc::VideoDecoderFactoryTemplate<
          webrtc::Dav1dDecoderTemplateAdapter>>(),
      nullptr /* audio_mixer */, nullptr /* audio_processing */);  
  

  if (!peer_connection_factory_) {
    RTC_LOG(LS_ERROR) << "Failed to initialize PeerConnectionFactory";
    if (!disable_gui_) {
      main_wnd_->MessageBox("Error", "Failed to initialize PeerConnectionFactory",
                            true);
    }
    DeletePeerConnection();
    return false;
  }

  // 2. create PeerConnection
  RTC_LOG(LS_INFO) << "2. Creating PeerConnection";
  if (!CreatePeerConnection()) {
    RTC_LOG(LS_ERROR) << "CreatePeerConnection failed";
    if (!disable_gui_) {
      main_wnd_->MessageBox("Error", "CreatePeerConnection failed", true);
    }
    DeletePeerConnection();
  }

  // 3. add audio and video tracks
  RTC_LOG(LS_INFO) << "3. Adding tracks";
  AddTracks();

  RTC_LOG(LS_INFO) << "Starting logging";
  StartLogging();

  RTC_LOG(LS_INFO) << __FUNCTION__ << ": Peer connection initialization "
                   << (peer_connection_ ? "successful" : "failed");
  return peer_connection_ != nullptr;
}

bool Conductor::ReinitializePeerConnectionForLoopback() {
  loopback_ = true;
  std::vector<rtc::scoped_refptr<webrtc::RtpSenderInterface>> senders =
      peer_connection_->GetSenders();
  peer_connection_ = nullptr;
  // Loopback is only possible if encryption is disabled.
  webrtc::PeerConnectionFactoryInterface::Options options;
  options.disable_encryption = true;
  peer_connection_factory_->SetOptions(options);
  if (CreatePeerConnection()) {
    for (const auto& sender : senders) {
      peer_connection_->AddTrack(sender->track(), sender->stream_ids());
    }
    peer_connection_->CreateOffer(
        this, webrtc::PeerConnectionInterface::RTCOfferAnswerOptions());
  }
  options.disable_encryption = false;
  peer_connection_factory_->SetOptions(options);
  return peer_connection_ != nullptr;
}

bool Conductor::CreatePeerConnection() {
  RTC_DCHECK(peer_connection_factory_);
  RTC_DCHECK(!peer_connection_);

  webrtc::PeerConnectionInterface::RTCConfiguration config;
  // format of sdp
  config.sdp_semantics = webrtc::SdpSemantics::kUnifiedPlan;

  // configure stun/turn server
  webrtc::PeerConnectionInterface::IceServer server;
  // server.uri = GetPeerConnectionString();
  server.uri = GetStunServerString();
  config.servers.push_back(server);

  // create peer_connection_ object
  webrtc::PeerConnectionDependencies pc_dependencies(this);
  auto error_or_peer_connection =
      peer_connection_factory_->CreatePeerConnectionOrError(
          config, std::move(pc_dependencies));
  if (error_or_peer_connection.ok()) {
    peer_connection_ = std::move(error_or_peer_connection.value());
  }
  return peer_connection_ != nullptr;
}

void Conductor::DeletePeerConnection() {
  StopLogging();
  main_wnd_->StopLocalRenderer();
  main_wnd_->StopRemoteRenderer();
  peer_connection_ = nullptr;
  peer_connection_factory_ = nullptr;
  peer_id_ = -1;
  loopback_ = false;
}

void Conductor::EnsureStreamingUI() {
  RTC_DCHECK(peer_connection_);
  if (main_wnd_->IsWindow()) {
    if (main_wnd_->current_ui() != MainWindow::STREAMING)
      main_wnd_->SwitchToStreamingUI();
  }
}

//
// PeerConnectionObserver implementation.
//

// After the above steps (send offer, send candidate, etc.), 
// the audio and video connection is established. 
// When a remote video stream is received, 
// the user will be notified through OnAddTrack().
void Conductor::OnAddTrack(
    rtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver,
    const std::vector<rtc::scoped_refptr<webrtc::MediaStreamInterface>>&
        streams) {
  RTC_LOG(LS_INFO) << __FUNCTION__ << " " << receiver->id();
  // send self-defined message NEW_TRACK_ADDED to MainWnd
  main_wnd_->QueueUIThreadCallback(NEW_TRACK_ADDED,
                                   receiver->track().release());
}

void Conductor::OnRemoveTrack(
    rtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver) {
  RTC_LOG(LS_INFO) << __FUNCTION__ << " " << receiver->id();
  main_wnd_->QueueUIThreadCallback(TRACK_REMOVED, receiver->track().release());
}

// After sending offer, caller will communicate ICE server to generate ice candidate, 
// and then send the ice candidate to callee through signaling server.
void Conductor::OnIceCandidate(const webrtc::IceCandidateInterface* candidate) {
  RTC_LOG(LS_INFO) << __FUNCTION__ << " " << candidate->sdp_mline_index();

  // For loopback test. To save some connecting delay.
  if (loopback_) {
    RTC_LOG(LS_INFO) << "Loopback mode: Adding ICE candidate locally";
    if (!peer_connection_->AddIceCandidate(candidate)) {
      RTC_LOG(LS_WARNING) << "Failed to apply the received candidate";
    }
    return;
  }

  // Package the ice candidate into JSON format.
  Json::Value jmessage;
  jmessage[kCandidateSdpMidName] = candidate->sdp_mid();
  jmessage[kCandidateSdpMlineIndexName] = candidate->sdp_mline_index();
  std::string sdp;
  if (!candidate->ToString(&sdp)) {
    RTC_LOG(LS_ERROR) << "Failed to serialize candidate";
    return;
  }
  jmessage[kCandidateSdpName] = sdp;

  // send the ice candidate to callee through signaling server
  Json::StreamWriterBuilder factory;
  std::string message = Json::writeString(factory, jmessage);
  
  RTC_LOG(LS_INFO) << "Sending ICE candidate: " << message.substr(0, 100) << (message.length() > 100 ? "..." : "");
  SendMessage(message);
}

//
// PeerConnectionClientObserver implementation.
//

void Conductor::OnSignedIn() {
  RTC_LOG(LS_INFO) << __FUNCTION__;
  my_id_ = client_->id(); 
  RTC_LOG(LS_INFO) << "Signed in with my_id_: " << my_id_;
  main_wnd_->SwitchToPeerList(client_->peers());
}

void Conductor::OnDisconnected() {
  RTC_LOG(LS_INFO) << __FUNCTION__;

  DeletePeerConnection();

  if (main_wnd_->IsWindow())
    main_wnd_->SwitchToConnectUI();
}

void Conductor::OnPeerConnected(int id, const std::string& name) {
  RTC_LOG(LS_INFO) << __FUNCTION__;
  // Refresh the list if we're showing it.
  if (!disable_gui_) {
    // Refresh the list if we're showing it.
    if (main_wnd_->current_ui() == MainWindow::LIST_PEERS)
      main_wnd_->SwitchToPeerList(client_->peers());
  }
  RTC_LOG(LS_INFO) << "Auto-connect check: disable_gui_=" << disable_gui_ 
                   << ", is_caller_=" << is_caller_ << ", peer_id_=" << peer_id_;
  if (disable_gui_ && is_caller_ && peer_id_ == -1) {
    // Automatically connect to the first peer in GUI-less mode
    ConnectToPeer(id);
  }  
}

void Conductor::OnPeerDisconnected(int id) {
  RTC_LOG(LS_INFO) << __FUNCTION__;
  if (id == peer_id_) {
    RTC_LOG(LS_INFO) << "Our peer disconnected";
    // inform the main_wnd_ that the peer connection is closed
    main_wnd_->QueueUIThreadCallback(PEER_CONNECTION_CLOSED, NULL);
  } else {
    // Refresh the list if we're showing it.
    if (main_wnd_->current_ui() == MainWindow::LIST_PEERS)
      main_wnd_->SwitchToPeerList(client_->peers());
  }
}

void Conductor::OnMessageFromPeer(int peer_id, const std::string& message) {
  if (!signaling_thread_.get()) {
    signaling_thread_ = rtc::Thread::CreateWithSocketServer();
    signaling_thread_->Start();
  }
  signaling_thread_->PostTask([=] { OnMessageFromPeerOnSignallingThread(peer_id, message); });
}

void Conductor::OnMessageFromPeerOnSignallingThread(int peer_id, const std::string& message) {  
  RTC_LOG(LS_INFO) << __FUNCTION__ << ": Received message from peer " << peer_id;
  RTC_DCHECK(peer_id_ == peer_id || peer_id_ == -1);
  RTC_DCHECK(!message.empty());

  // the callee hasn't create a PeerConnection object yet.
  if (!peer_connection_.get()) {
    RTC_LOG(LS_INFO) << "PeerConnection not created yet. Creating now.";
    RTC_DCHECK(peer_id_ == -1);
    peer_id_ = peer_id;

    // create PeerConnection object
    if (!InitializePeerConnection()) {
      RTC_LOG(LS_ERROR) << "Failed to initialize our PeerConnection instance";
      client_->SignOut();
      return;
    }
  } else if (peer_id != peer_id_) {
    RTC_DCHECK(peer_id_ != -1);
    RTC_LOG(LS_WARNING)
        << "Received a message from unknown peer while already in a "
           "conversation with a different peer.";
    return;
  }

  Json::CharReaderBuilder factory;
  std::unique_ptr<Json::CharReader> reader =
      absl::WrapUnique(factory.newCharReader());
  Json::Value jmessage;
  // parse the message to json object
  if (!reader->parse(message.data(), message.data() + message.length(),
                     &jmessage, nullptr)) {
    RTC_LOG(LS_WARNING) << "Received unknown message. " << message;
    return;
  }
  std::string type_str;
  std::string json_object;

  // get the type of the message from json
  rtc::GetStringFromJsonObject(jmessage, kSessionDescriptionTypeName,
                               &type_str);
  if (!type_str.empty()) {
    RTC_LOG(LS_INFO) << "Received message type: " << type_str;
    if (type_str == "offer-loopback") {
      RTC_LOG(LS_INFO) << "Received loopback offer. Reinitializing PeerConnection.";
      // This is a loopback call.
      // Recreate the peerconnection with DTLS disabled.
      if (!ReinitializePeerConnectionForLoopback()) {
        RTC_LOG(LS_ERROR) << "Failed to initialize our PeerConnection instance";
        DeletePeerConnection();
        client_->SignOut();
      }
      return;
    }
    // type of the message
    absl::optional<webrtc::SdpType> type_maybe =
        webrtc::SdpTypeFromString(type_str);
    if (!type_maybe) {
      RTC_LOG(LS_ERROR) << "Unknown SDP type: " << type_str;
      return;
    }
    webrtc::SdpType type = *type_maybe;
    std::string sdp;

    // get the sdp from json, which is the offer here.
    if (!rtc::GetStringFromJsonObject(jmessage, kSessionDescriptionSdpName,
                                      &sdp)) {
      RTC_LOG(LS_WARNING)
          << "Can't parse received session description message.";
      return;
    }

    // parse the sdp to SessionDescriptionInterface object, which is understandable by WebRTC
    webrtc::SdpParseError error;
    std::unique_ptr<webrtc::SessionDescriptionInterface> session_description =
        webrtc::CreateSessionDescription(type, sdp, &error);
    if (!session_description) {
      RTC_LOG(LS_WARNING)
          << "Can't parse received session description message. "
             "SdpParseError was: "
          << error.description;
      return;
    }
    RTC_LOG(LS_INFO) << "Received session description: " << type_str;

    // set the sdp to PeerConnection through SetRemoteDescription
    peer_connection_->SetRemoteDescription(
        DummySetSessionDescriptionObserver::Create().get(),
        session_description.release());

    // after receiving the offer, create answer.
    // CreateAnswer is also async, it will call OnSuccess or OnFailure when finishes.
    if (type == webrtc::SdpType::kOffer) {
      RTC_LOG(LS_INFO) << "Creating answer to received offer";
      peer_connection_->CreateAnswer(
          this, webrtc::PeerConnectionInterface::RTCOfferAnswerOptions());
    }
  } else {
    RTC_LOG(LS_INFO) << "Received ICE candidate";
    // handle the ice candidate message
    std::string sdp_mid;
    int sdp_mlineindex = 0;
    std::string sdp;

    // parse the ice candidate message from json
    if (!rtc::GetStringFromJsonObject(jmessage, kCandidateSdpMidName,
                                      &sdp_mid) ||
        !rtc::GetIntFromJsonObject(jmessage, kCandidateSdpMlineIndexName,
                                   &sdp_mlineindex) ||
        !rtc::GetStringFromJsonObject(jmessage, kCandidateSdpName, &sdp)) {
      RTC_LOG(LS_WARNING) << "Can't parse received message.";
      return;
    }
    webrtc::SdpParseError error;

    // generate IceCandidate object from received message
    std::unique_ptr<webrtc::IceCandidateInterface> candidate(
        webrtc::CreateIceCandidate(sdp_mid, sdp_mlineindex, sdp, &error));
    if (!candidate.get()) {
      RTC_LOG(LS_WARNING) << "Can't parse received candidate message. "
                             "SdpParseError was: "
                          << error.description;
      return;
    }

    // apply the received candidate to PeerConnection
    if (!peer_connection_->AddIceCandidate(candidate.get())) {
      RTC_LOG(LS_WARNING) << "Failed to apply the received candidate";
      return;
    }
    RTC_LOG(LS_INFO) << "Successfully added ICE candidate";
  }

  RTC_LOG(LS_INFO) << __FUNCTION__ << ": Finished processing message";
}

void Conductor::OnMessageSent(int err) {
  // Process the next pending message if any.
  main_wnd_->QueueUIThreadCallback(SEND_MESSAGE_TO_PEER, NULL);
}

void Conductor::OnServerConnectionFailure() {
  main_wnd_->MessageBox("Error", ("Failed to connect to " + server_).c_str(),
                        true);
}

//
// MainWndCallback implementation.
//

// connect to signaling server, it calls PeerConnectionClient::Connect() function
void Conductor::StartLogin(const std::string& server, int port) {
  if (client_->is_connected())
    return;
  server_ = server;
  client_->Connect(server, port, GetPeerName());
}

void Conductor::AutoLogin(const std::string& server, int port) {
  if (disable_gui_) {
    StartLogin(server, port);
  }
}

void Conductor::DisconnectFromServer() {
  if (client_->is_connected())
    client_->SignOut();
}

// Clicking on a user ID in the peer_list ui will trigger this function. 
// A crucial operation in this function is calling the CreateOffer() function.
void Conductor::ConnectToPeer(int peer_id) {
  RTC_LOG(LS_INFO) << __FUNCTION__ << " peer_id: " << peer_id << " current peer_id_: " << peer_id_;

  if (peer_id_ != -1) {
    RTC_LOG(LS_WARNING) << "Already connected to a peer. Ignoring connection attempt.";
    return;
  }
  
  if (peer_connection_.get()) {
    if (!disable_gui_) {
      main_wnd_->MessageBox(
          "Error", "We only support connecting to one peer at a time", true);
    } else {
      RTC_LOG(LS_WARNING) << "PeerConnection already exists. Ignoring connection attempt.";
    }
    return;
  }

  if (InitializePeerConnection()) {
    peer_id_ = peer_id;
    peer_connection_->CreateOffer(
        this, webrtc::PeerConnectionInterface::RTCOfferAnswerOptions());
  } else {
    if (!disable_gui_) {
      main_wnd_->MessageBox("Error", "Failed to initialize PeerConnection", true);
    } else {
      RTC_LOG(LS_ERROR) << "Failed to initialize PeerConnection";
    }
  }
}

void Conductor::AddTracks() {
  RTC_LOG(LS_INFO) << "Entering AddTracks()";
  if (!peer_connection_->GetSenders().empty()) {
    RTC_LOG(LS_INFO) << "Tracks already added. Exiting AddTracks()";
    return;  // Already added tracks.
  }

  // create audio track
  RTC_LOG(LS_INFO) << "Creating audio track";
  rtc::scoped_refptr<webrtc::AudioTrackInterface> audio_track(
      peer_connection_factory_->CreateAudioTrack(
          kAudioLabel,
          peer_connection_factory_->CreateAudioSource(cricket::AudioOptions())
              .get()));
  // add audio track to PeerConnection
  RTC_LOG(LS_INFO) << "Adding audio track to PeerConnection";
  auto result_or_error = peer_connection_->AddTrack(audio_track, {kStreamId});
  if (!result_or_error.ok()) {
    RTC_LOG(LS_ERROR) << "Failed to add audio track to PeerConnection: "
                      << result_or_error.error().message();
  }

  // create video device
  // uses webrtc::VideoCaptureFactory::CreateDeviceInfo() to get the video device info.
  RTC_LOG(LS_INFO) << "Creating video device";
  rtc::scoped_refptr<CapturerTrackSource> video_device =
      CapturerTrackSource::Create();
  if (video_device) {
    RTC_LOG(LS_INFO) << "Video device created successfully";
    // create video track
    RTC_LOG(LS_INFO) << "Creating video track";
    rtc::scoped_refptr<webrtc::VideoTrackInterface> video_track_(
        peer_connection_factory_->CreateVideoTrack(video_device, kVideoLabel));
    // send video track to local renderer
    main_wnd_->StartLocalRenderer(video_track_.get(), my_id_);

    // add video track to PeerConnection
    result_or_error = peer_connection_->AddTrack(video_track_, {kStreamId});
    if (!result_or_error.ok()) {
      RTC_LOG(LS_ERROR) << "Failed to add video track to PeerConnection: "
                        << result_or_error.error().message();
    }
  } else {
    RTC_LOG(LS_ERROR) << "OpenVideoCaptureDevice failed";
  }
  
  // switch to streaming UI
  main_wnd_->SwitchToStreamingUI();
  RTC_LOG(LS_INFO) << "Exiting AddTracks()";
}

// Press esc key, and function GtkMainWnd::OnKeyPress captures this event.
void Conductor::DisconnectFromCurrentPeer() {
  RTC_LOG(LS_INFO) << __FUNCTION__;
  if (peer_connection_.get()) {
    client_->SendHangUp(peer_id_);  // send bye signal to peer through server
    DeletePeerConnection();
  }

  if (main_wnd_->IsWindow())
    main_wnd_->SwitchToPeerList(client_->peers());  // switch to peer_list UI
}

void Conductor::QueuePendingMessage(int msg_id, void* data) {
  pending_messages_NonGUI.push_back(new UIThreadCallbackData(msg_id, data));
}

void Conductor::ProcessMessagesForNonGUIMode() {
  while (!pending_messages_NonGUI.empty()) {
    // Pop the first message from the queue.
    UIThreadCallbackData* data = pending_messages_NonGUI.front();
    pending_messages_NonGUI.pop_front();

    // Process the message using UIThreadCallback.
    UIThreadCallback(data->msg_id, data->data);

    // Clean up the data.
    delete data;
  }
}



// After posting the message to MainWnd by Conductor::SendMessage, 
// it will be handled in this function, which runs on the main thread.
void Conductor::UIThreadCallback(int msg_id, void* data) {
  RTC_LOG(LS_INFO) << "Entering UIThreadCallback with msg_id: " << msg_id;

  switch (msg_id) {
    case PEER_CONNECTION_CLOSED:
      RTC_LOG(LS_INFO) << "PEER_CONNECTION_CLOSED";
      DeletePeerConnection();

      if (main_wnd_->IsWindow()) {      // if in video streaming UI
        if (client_->is_connected()) {  // if connected to signaling server, switch to peer_list UI
          main_wnd_->SwitchToPeerList(client_->peers());
        } else {
          main_wnd_->SwitchToConnectUI(); // siwtch to connect UI
        }
      } else {
        DisconnectFromServer();
      }
      break;

    // send message to server
    case SEND_MESSAGE_TO_PEER: {
      RTC_LOG(LS_INFO) << __FUNCTION__ << " SEND_MESSAGE_TO_PEER";
      // obtain message
      std::string* msg = reinterpret_cast<std::string*>(data);
      if (msg) {
        // For convenience, we always run the message through the queue.
        // This way we can be sure that messages are sent to the server
        // in the same order they were signaled without much hassle.
        pending_messages_.push_back(msg);
      }

      // If there is a message to be sent and PeerConnectionClient 
      // is available to send signaling.
      if (!pending_messages_.empty() && !client_->IsSendingMessage()) {
        msg = pending_messages_.front(); // obtain the message from the queue
        pending_messages_.pop_front();

        // send message through PeerConnectionClient to server
        if (!client_->SendToPeer(peer_id_, *msg) && peer_id_ != -1) {
          RTC_LOG(LS_ERROR) << "SendToPeer failed";
          DisconnectFromServer();
        }
        delete msg;
      }

      if (!peer_connection_.get())
        peer_id_ = -1;

      break;
    }

    case NEW_TRACK_ADDED: {
      auto* track = reinterpret_cast<webrtc::MediaStreamTrackInterface*>(data);
      if (track->kind() == webrtc::MediaStreamTrackInterface::kVideoKind) {
        // obtain remote video track
        auto* video_track = static_cast<webrtc::VideoTrackInterface*>(track);
        // send remote video track to main_wnd_ renderer
        RTC_LOG(LS_INFO) << __FUNCTION__ << "New video track added, starting Remote Renderer.";
        main_wnd_->StartRemoteRenderer(video_track, my_id_);
      }
      track->Release();
      break;
    }

    case TRACK_REMOVED: {
      // Remote peer stopped sending a track.
      auto* track = reinterpret_cast<webrtc::MediaStreamTrackInterface*>(data);
      track->Release();
      break;
    }

    default:
      RTC_LOG(LS_WARNING) << "Unhandled msg_id in UIThreadCallback: " << msg_id;
      RTC_DCHECK_NOTREACHED();
      break;
  }

  RTC_LOG(LS_INFO) << "Exiting UIThreadCallback";
}

// After WebRTC generates an offer, it will be passed back through this callback 
// function. In this function, the offer will be formatted into a JSON message, 
// and then the SendMessage function will be called to send the message.
void Conductor::OnSuccess(webrtc::SessionDescriptionInterface* desc) {
  // The generated offer needs to be set in the PeerConnection using SetLocalDescription.
  // SetLocalDescription is also an async operation, 
  // will call DummySetSessionDescriptionObserver::OnSuccess when finished.
  RTC_LOG(LS_INFO) << "Entering OnSuccess with description type: " << webrtc::SdpTypeToString(desc->GetType());
  
  std::string sdp;
  desc->ToString(&sdp);  // desc is the offer, it is formatted into a string
  LogLocalSDP(webrtc::SdpTypeToString(desc->GetType()), sdp);  
  
  peer_connection_->SetLocalDescription(
      DummySetSessionDescriptionObserver::Create().get(), desc);

  // For loopback test. To save some connecting delay.
  if (loopback_) {
    // Replace message type from "offer" to "answer"
    RTC_LOG(LS_INFO) << "Loopback mode: Creating answer from offer";
    std::unique_ptr<webrtc::SessionDescriptionInterface> session_description =
        webrtc::CreateSessionDescription(webrtc::SdpType::kAnswer, sdp);
    peer_connection_->SetRemoteDescription(
        DummySetSessionDescriptionObserver::Create().get(),
        session_description.release());
    return;
  }

  // Package the offer into JSON format.
  Json::Value jmessage;
  jmessage[kSessionDescriptionTypeName] =
      webrtc::SdpTypeToString(desc->GetType());
  jmessage[kSessionDescriptionSdpName] = sdp;

  Json::StreamWriterBuilder factory;
  std::string json_message = Json::writeString(factory, jmessage);
  RTC_LOG(LS_INFO) << "Sending message to peer: " << json_message.substr(0, 100) << "..."; // Log first 100 chars of message  
  // send the offer signal to the server
  SendMessage(Json::writeString(factory, jmessage));

  RTC_LOG(LS_INFO) << "Exiting OnSuccess";
}

// if the offer generation fails, this function will be called.
void Conductor::OnFailure(webrtc::RTCError error) {
  RTC_LOG(LS_ERROR) << ToString(error.type()) << ": " << error.message();
}

// At this point, it is still within the WebRTC internal thread, 
// but the signaling must be sent by the main thread. Therefore, 
// the signaling to be sent is posted to MainWnd, allowing it 
// to send the signaling.
void Conductor::SendMessage(const std::string& json_object) {
  RTC_LOG(LS_INFO) << "Entering Conductor::SendMessage.";
  std::string* msg = new std::string(json_object);
  // message type is SEND_MESSAGE_TO_PEER
  main_wnd_->QueueUIThreadCallback(SEND_MESSAGE_TO_PEER, msg);
}


void Conductor::LogLocalSDP(const std::string& type, const std::string& sdp) {
    if (!sdp_log_) {
        std::string filename = "sdp_" + std::to_string(my_id_) + ".txt";
        sdp_log_ = std::make_unique<std::ofstream>(filename, std::ios::app);
    }
    
    if (sdp_log_->is_open()) {
        *sdp_log_ << "Time: " << rtc::TimeMillis() << "\n"
                 << "LocalDescription Type: " << type << "\n"
                 << "SDP:\n" << sdp << "\n"
                 << "----------------------------------------\n";
        sdp_log_->flush();
    }
}

// New methods for logging
void Conductor::StartLogging() {
  if (!logging_active_.exchange(true)) {
    start_time_ = static_cast<unsigned long>(
        std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count());
    
    std::string prefix = std::to_string(start_time_);
    char filename[256];
    snprintf(filename, sizeof(filename), "%s-pc-%d.csv", prefix.c_str(), my_id_);
    pc_log_ = std::make_unique<CSVWriter>(filename);
    snprintf(filename, sizeof(filename), "%s-in-rtp-%d.csv", prefix.c_str(), my_id_);
    in_rtp_log_ = std::make_unique<CSVWriter>(filename);
    snprintf(filename, sizeof(filename), "%s-out-rtp-%d.csv", prefix.c_str(), my_id_);
    out_rtp_log_ = std::make_unique<CSVWriter>(filename);

    // Peer connection stats header
    *pc_log_ << "timestamp,transport_id,local_candidate_id,remote_candidate_id,state,priority,"
             << "nominated,writable,packets_sent,packets_received,bytes_sent,bytes_received,"
             << "total_round_trip_time,current_round_trip_time,available_outgoing_bitrate,"
             << "available_incoming_bitrate,requests_received,requests_sent,responses_received,"
             << "responses_sent,consent_requests_sent,packets_discarded_on_send,"
             << "bytes_discarded_on_send,last_packet_received_timestamp,last_packet_sent_timestamp\n";

    // Inbound RTP stats header
    *in_rtp_log_ << "timestamp,track_identifier,mid,remote_id,ssrc,kind,packets_lost,packets_received,"
                 << "packets_discarded,fec_packets_received,fec_bytes_received,bytes_received,"
                 << "header_bytes_received,retransmitted_packets_received,retransmitted_bytes_received,"
                 << "rtx_ssrc,jitter,jitter_buffer_delay,jitter_buffer_target_delay,"
                 << "jitter_buffer_minimum_delay,jitter_buffer_emitted_count,total_samples_received,"
                 << "concealed_samples,silent_concealed_samples,concealment_events,"
                 << "inserted_samples_for_deceleration,removed_samples_for_acceleration,audio_level,"
                 << "total_audio_energy,total_samples_duration,frames_received,frame_width,frame_height,"
                 << "frames_per_second,frames_decoded,key_frames_decoded,frames_dropped,total_decode_time,"
                 << "total_processing_delay,total_assembly_time,frames_assembled_from_multiple_packets,"
                 << "total_inter_frame_delay,total_squared_inter_frame_delay,pause_count,"
                 << "total_pauses_duration,freeze_count,total_freezes_duration,content_type,"
                 << "estimated_playout_timestamp,decoder_implementation,fir_count,pli_count,nack_count,"
                 << "qp_sum,jitter_buffer_flushes,delayed_packet_outage_samples,"
                 << "relative_packet_arrival_delay,interruption_count,total_interruption_duration,"
                 << "min_playout_delay\n";

    // Outbound RTP stats header
    *out_rtp_log_ << "timestamp,media_source_id,remote_id,mid,rid,ssrc,rtx_ssrc,kind,packets_sent,"
                  << "bytes_sent,retransmitted_packets_sent,header_bytes_sent,retransmitted_bytes_sent,"
                  << "target_bitrate,frames_encoded,key_frames_encoded,total_encode_time,"
                  << "total_encoded_bytes_target,frame_width,frame_height,frames_per_second,frames_sent,"
                  << "huge_frames_sent,total_packet_send_delay,quality_limitation_reason,"
                  << "quality_limitation_duration_bandwidth,quality_limitation_duration_cpu,"
                  << "quality_limitation_duration_none,quality_limitation_duration_other,"
                  << "quality_limitation_resolution_changes,content_type,encoder_implementation,"
                  << "fir_count,pli_count,nack_count,qp_sum,active,power_efficient_encoder,"
                  << "scalability_mode\n";

    // start logging thread
    logging_thread_ = std::make_unique<std::thread>(&Conductor::LoggingThread, this);
  }
}

void Conductor::StopLogging() {
  if (logging_active_.exchange(false)) {
    if (logging_thread_ && logging_thread_->joinable()) {
      logging_thread_->join();
    }
    pc_log_.reset();
    in_rtp_log_.reset();
    out_rtp_log_.reset();
  }
}

void Conductor::LoggingThread() {
  while (logging_active_) {
    LogMetrics();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }
}

// Creates a StatsCallback object and passes it to peer_connection_->GetStats().
void Conductor::LogMetrics() {
  if (!peer_connection_) return;

  // RefCountedObject is a template class, StatsCallback is the type 
  // we're "specializing" the template with. We're creating a new 
  // object that is a StatsCallback, and has additional reference 
  // counting functionality.
  rtc::scoped_refptr<StatsCallback> callback = 
      rtc::scoped_refptr<StatsCallback>(
          new rtc::RefCountedObject<StatsCallback>(this));
  // When we pass our StatsCallback object to GetStats(), WebRTC 
  // stores it as a RTCStatsCollectorCallback*. When the stats are 
  // ready, it calls OnStatsDelivered() on this pointer.  
  // The get() method is defined in the rtc::scoped_refptr class, 
  // which returns the raw pointer managed by the scoped_refptr.
  // This design using scoped_refptr and RefCountedObject is common 
  // in WebRTC for managing object lifetimes and reference counting.    
  // Asynchronous Method: it starts collecting stats but doesn't 
  // wait for them to be ready before returning.
  peer_connection_->GetStats(callback.get());
}

// Called by StatsCallback::OnStatsDelivered()
// It uses GetStatsOfType<T>() to get specific types of stats. 
void Conductor::OnStatsDelivered(
    const rtc::scoped_refptr<const webrtc::RTCStatsReport>& report) {
  std::lock_guard<std::mutex> lock(log_mutex_);
  uint64_t timestamp = rtc::TimeMillis();

  // Log peer connection stats
  auto candidate_pair_stats = report->GetStatsOfType<webrtc::RTCIceCandidatePairStats>();
  if (!candidate_pair_stats.empty()) {
    // Find the candidate pair with availableOutgoingBitrate
    const webrtc::RTCIceCandidatePairStats* chosen_stat = nullptr;
    for (const auto& stat : candidate_pair_stats) {
      if (stat->available_outgoing_bitrate.has_value()) {
        chosen_stat = stat;
        break;
      }
    }

    // If we found a candidate pair with availableOutgoingBitrate, log it
    if (chosen_stat) {
      *pc_log_ << timestamp << ",";
      pc_log_->writeFromDict(*chosen_stat, "transport_id");
      pc_log_->writeFromDict(*chosen_stat, "local_candidate_id");
      pc_log_->writeFromDict(*chosen_stat, "remote_candidate_id");
      pc_log_->writeFromDict(*chosen_stat, "state");
      pc_log_->writeFromDict(*chosen_stat, "priority");
      pc_log_->writeFromDict(*chosen_stat, "nominated");
      pc_log_->writeFromDict(*chosen_stat, "writable");
      pc_log_->writeFromDict(*chosen_stat, "packets_sent");
      pc_log_->writeFromDict(*chosen_stat, "packets_received");
      pc_log_->writeFromDict(*chosen_stat, "bytes_sent");
      pc_log_->writeFromDict(*chosen_stat, "bytes_received");
      pc_log_->writeFromDict(*chosen_stat, "total_round_trip_time");
      pc_log_->writeFromDict(*chosen_stat, "current_round_trip_time");
      pc_log_->writeFromDict(*chosen_stat, "available_outgoing_bitrate"); // bps
      pc_log_->writeFromDict(*chosen_stat, "available_incoming_bitrate");
      pc_log_->writeFromDict(*chosen_stat, "requests_received");
      pc_log_->writeFromDict(*chosen_stat, "requests_sent");
      pc_log_->writeFromDict(*chosen_stat, "responses_received");
      pc_log_->writeFromDict(*chosen_stat, "responses_sent");
      pc_log_->writeFromDict(*chosen_stat, "consent_requests_sent");
      pc_log_->writeFromDict(*chosen_stat, "packets_discarded_on_send");
      pc_log_->writeFromDict(*chosen_stat, "bytes_discarded_on_send");
      pc_log_->writeFromDict(*chosen_stat, "last_packet_received_timestamp");
      pc_log_->writeFromDict(*chosen_stat, "last_packet_sent_timestamp");
      *pc_log_ << "\n";
    } else {
      // Log a message if no candidate pair with availableOutgoingBitrate was found
      RTC_LOG(LS_WARNING) << "No candidate pair with availableOutgoingBitrate found";
    }
  }

  // RTC_LOG(LS_INFO) << "Available outgoing bandwidth: " << candidate_pair_stats[0]->available_outgoing_bitrate.value() << " bps";  

  // Log inbound RTP stats
  auto inbound_rtp_stats = report->GetStatsOfType<webrtc::RTCInboundRtpStreamStats>();
  for (const auto& stat : inbound_rtp_stats) {
    *in_rtp_log_ << timestamp << ",";
    in_rtp_log_->writeFromDict(*stat, "track_identifier");
    in_rtp_log_->writeFromDict(*stat, "mid");
    in_rtp_log_->writeFromDict(*stat, "remote_id");
    in_rtp_log_->writeFromDict(*stat, "ssrc");  // This might be from the base class
    in_rtp_log_->writeFromDict(*stat, "kind");  // This might be from the base class
    in_rtp_log_->writeFromDict(*stat, "packets_lost");  // This might be from the base class
    in_rtp_log_->writeFromDict(*stat, "packets_received");
    in_rtp_log_->writeFromDict(*stat, "packets_discarded");
    in_rtp_log_->writeFromDict(*stat, "fec_packets_received");
    in_rtp_log_->writeFromDict(*stat, "fec_bytes_received");
    in_rtp_log_->writeFromDict(*stat, "bytes_received");
    in_rtp_log_->writeFromDict(*stat, "header_bytes_received");
    in_rtp_log_->writeFromDict(*stat, "retransmitted_packets_received");
    in_rtp_log_->writeFromDict(*stat, "retransmitted_bytes_received");
    in_rtp_log_->writeFromDict(*stat, "rtx_ssrc");
    in_rtp_log_->writeFromDict(*stat, "jitter");  // This might be from the base class
    in_rtp_log_->writeFromDict(*stat, "jitter_buffer_delay");
    in_rtp_log_->writeFromDict(*stat, "jitter_buffer_target_delay");
    in_rtp_log_->writeFromDict(*stat, "jitter_buffer_minimum_delay");
    in_rtp_log_->writeFromDict(*stat, "jitter_buffer_emitted_count");
    in_rtp_log_->writeFromDict(*stat, "total_samples_received");
    in_rtp_log_->writeFromDict(*stat, "concealed_samples");
    in_rtp_log_->writeFromDict(*stat, "silent_concealed_samples");
    in_rtp_log_->writeFromDict(*stat, "concealment_events");
    in_rtp_log_->writeFromDict(*stat, "inserted_samples_for_deceleration");
    in_rtp_log_->writeFromDict(*stat, "removed_samples_for_acceleration");
    in_rtp_log_->writeFromDict(*stat, "audio_level");
    in_rtp_log_->writeFromDict(*stat, "total_audio_energy");
    in_rtp_log_->writeFromDict(*stat, "total_samples_duration");
    in_rtp_log_->writeFromDict(*stat, "frames_received");
    in_rtp_log_->writeFromDict(*stat, "frame_width");
    in_rtp_log_->writeFromDict(*stat, "frame_height");
    in_rtp_log_->writeFromDict(*stat, "frames_per_second");
    in_rtp_log_->writeFromDict(*stat, "frames_decoded");
    in_rtp_log_->writeFromDict(*stat, "key_frames_decoded");
    in_rtp_log_->writeFromDict(*stat, "frames_dropped");
    in_rtp_log_->writeFromDict(*stat, "total_decode_time");
    in_rtp_log_->writeFromDict(*stat, "total_processing_delay");
    in_rtp_log_->writeFromDict(*stat, "total_assembly_time");
    in_rtp_log_->writeFromDict(*stat, "frames_assembled_from_multiple_packets");
    in_rtp_log_->writeFromDict(*stat, "total_inter_frame_delay");
    in_rtp_log_->writeFromDict(*stat, "total_squared_inter_frame_delay");
    in_rtp_log_->writeFromDict(*stat, "pause_count");
    in_rtp_log_->writeFromDict(*stat, "total_pauses_duration");
    in_rtp_log_->writeFromDict(*stat, "freeze_count");
    in_rtp_log_->writeFromDict(*stat, "total_freezes_duration");
    in_rtp_log_->writeFromDict(*stat, "content_type");
    in_rtp_log_->writeFromDict(*stat, "estimated_playout_timestamp");
    in_rtp_log_->writeFromDict(*stat, "decoder_implementation");
    in_rtp_log_->writeFromDict(*stat, "fir_count");
    in_rtp_log_->writeFromDict(*stat, "pli_count");
    in_rtp_log_->writeFromDict(*stat, "nack_count");
    in_rtp_log_->writeFromDict(*stat, "qp_sum");
    in_rtp_log_->writeFromDict(*stat, "jitter_buffer_flushes");
    in_rtp_log_->writeFromDict(*stat, "delayed_packet_outage_samples");
    in_rtp_log_->writeFromDict(*stat, "relative_packet_arrival_delay");
    in_rtp_log_->writeFromDict(*stat, "interruption_count");
    in_rtp_log_->writeFromDict(*stat, "total_interruption_duration");
    in_rtp_log_->writeFromDict(*stat, "min_playout_delay");
    *in_rtp_log_ << "\n";
  }

  // Log outbound RTP stats
  auto outbound_rtp_stats = report->GetStatsOfType<webrtc::RTCOutboundRtpStreamStats>();
  for (const auto& stat : outbound_rtp_stats) {
    *out_rtp_log_ << timestamp << ",";
    out_rtp_log_->writeFromDict(*stat, "media_source_id");
    out_rtp_log_->writeFromDict(*stat, "remote_id");
    out_rtp_log_->writeFromDict(*stat, "mid");
    out_rtp_log_->writeFromDict(*stat, "rid");
    out_rtp_log_->writeFromDict(*stat, "ssrc");  // This might be from the base class
    out_rtp_log_->writeFromDict(*stat, "rtx_ssrc");
    out_rtp_log_->writeFromDict(*stat, "kind");  // This might be from the base class
    out_rtp_log_->writeFromDict(*stat, "packets_sent");  // This might be from the base class
    out_rtp_log_->writeFromDict(*stat, "bytes_sent");  // This might be from the base class
    out_rtp_log_->writeFromDict(*stat, "retransmitted_packets_sent");
    out_rtp_log_->writeFromDict(*stat, "header_bytes_sent");
    out_rtp_log_->writeFromDict(*stat, "retransmitted_bytes_sent");
    out_rtp_log_->writeFromDict(*stat, "target_bitrate");
    out_rtp_log_->writeFromDict(*stat, "frames_encoded");
    out_rtp_log_->writeFromDict(*stat, "key_frames_encoded");
    out_rtp_log_->writeFromDict(*stat, "total_encode_time");
    out_rtp_log_->writeFromDict(*stat, "total_encoded_bytes_target");
    out_rtp_log_->writeFromDict(*stat, "frame_width");
    out_rtp_log_->writeFromDict(*stat, "frame_height");
    out_rtp_log_->writeFromDict(*stat, "frames_per_second");
    out_rtp_log_->writeFromDict(*stat, "frames_sent");
    out_rtp_log_->writeFromDict(*stat, "huge_frames_sent");
    out_rtp_log_->writeFromDict(*stat, "total_packet_send_delay");
    out_rtp_log_->writeFromDict(*stat, "quality_limitation_reason");
    
    // Handle quality_limitation_durations
    if (stat->quality_limitation_durations.has_value()) {
      const auto& durations = *stat->quality_limitation_durations;
      *out_rtp_log_ << durations.find("bandwidth")->second << ","
                    << durations.find("cpu")->second << ","
                    << durations.find("none")->second << ","
                    << durations.find("other")->second << ",";
    } else {
      *out_rtp_log_ << ",,,," ;  // Empty values if not present
    }
    
    out_rtp_log_->writeFromDict(*stat, "quality_limitation_resolution_changes");
    out_rtp_log_->writeFromDict(*stat, "content_type");
    out_rtp_log_->writeFromDict(*stat, "encoder_implementation");
    out_rtp_log_->writeFromDict(*stat, "fir_count");
    out_rtp_log_->writeFromDict(*stat, "pli_count");
    out_rtp_log_->writeFromDict(*stat, "nack_count");
    out_rtp_log_->writeFromDict(*stat, "qp_sum");
    out_rtp_log_->writeFromDict(*stat, "active");
    out_rtp_log_->writeFromDict(*stat, "power_efficient_encoder");
    out_rtp_log_->writeFromDict(*stat, "scalability_mode");
    *out_rtp_log_ << "\n";
  }

  pc_log_->flush();
  in_rtp_log_->flush();
  out_rtp_log_->flush();
}

