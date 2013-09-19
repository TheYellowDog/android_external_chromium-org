// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "base/bind.h"
#include "base/memory/ref_counted.h"
#include "base/memory/scoped_ptr.h"
#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "media/cast/audio_receiver/audio_decoder.h"
#include "media/cast/cast_thread.h"

namespace media {
namespace cast {

static const int64 kStartMillisecond = 123456789;

class AudioDecoderTest : public ::testing::Test {
 protected:
  AudioDecoderTest() {}

  ~AudioDecoderTest() {}

  virtual void SetUp() {
    cast_thread_ = new CastThread(MessageLoopProxy::current(),
                                  MessageLoopProxy::current(),
                                  MessageLoopProxy::current(),
                                  MessageLoopProxy::current(),
                                  MessageLoopProxy::current());
  }
  void Configure(const AudioReceiverConfig& audio_config) {
    audio_decoder_ = new AudioDecoder(cast_thread_, audio_config);
  }

  // Used in MessageLoopProxy::current().
  base::MessageLoop loop_;
  scoped_refptr<CastThread> cast_thread_;
  scoped_refptr<AudioDecoder> audio_decoder_;
};

TEST_F(AudioDecoderTest, Pcm16MonoNoResampleOnePacket) {
  base::RunLoop run_loop;
  AudioReceiverConfig audio_config;
  audio_config.rtp_payload_type = 127;
  audio_config.frequency = 16000;
  audio_config.channels = 1;
  audio_config.codec = kPcm16;
  audio_config.use_external_decoder = false;
  Configure(audio_config);

  RtpCastHeader rtp_header;
  rtp_header.webrtc.header.payloadType = 127;
  rtp_header.webrtc.header.sequenceNumber = 1234;
  rtp_header.webrtc.header.timestamp = 0x87654321;
  rtp_header.webrtc.header.ssrc = 0x12345678;
  rtp_header.webrtc.header.paddingLength = 0;
  rtp_header.webrtc.header.headerLength = 12;
  rtp_header.webrtc.type.Audio.channel = 1;
  rtp_header.webrtc.type.Audio.isCNG = false;

  std::vector<int16> payload(640, 0x1234);

  uint8* payload_data = reinterpret_cast<uint8*>(&payload[0]);
  int payload_size = payload.size() * sizeof(int16);

  audio_decoder_->IncomingParsedRtpPacket(payload_data, payload_size,
                                          rtp_header);

  int number_of_10ms_blocks = 4;
  int desired_frequency = 16000;
  PcmAudioFrame audio_frame;
  uint32 rtp_timestamp;

  EXPECT_TRUE(audio_decoder_->GetRawAudioFrame(number_of_10ms_blocks,
                                               desired_frequency,
                                               &audio_frame,
                                               &rtp_timestamp));

  EXPECT_EQ(1, audio_frame.channels);
  EXPECT_EQ(16000, audio_frame.frequency);
  EXPECT_EQ(640ul, audio_frame.samples.size());
  // First 10 samples per channel are 0 from NetEq.
  for (size_t i = 10; i < audio_frame.samples.size(); ++i) {
    EXPECT_EQ(0x3412, audio_frame.samples[i]);
  }
  run_loop.RunUntilIdle();
}

TEST_F(AudioDecoderTest, Pcm16StereoNoResampleTwoPackets) {
  base::RunLoop run_loop;
  AudioReceiverConfig audio_config;
  audio_config.rtp_payload_type = 127;
  audio_config.frequency = 16000;
  audio_config.channels = 2;
  audio_config.codec = kPcm16;
  audio_config.use_external_decoder = false;
  Configure(audio_config);

  RtpCastHeader rtp_header;
  rtp_header.webrtc.header.payloadType = 127;
  rtp_header.webrtc.header.sequenceNumber = 1234;
  rtp_header.webrtc.header.timestamp = 0x87654321;
  rtp_header.webrtc.header.ssrc = 0x12345678;
  rtp_header.webrtc.header.paddingLength = 0;
  rtp_header.webrtc.header.headerLength = 12;

  rtp_header.webrtc.type.Audio.isCNG = false;
  rtp_header.webrtc.type.Audio.channel = 2;

  std::vector<int16> payload(640, 0x1234);

  uint8* payload_data = reinterpret_cast<uint8*>(&payload[0]);
  int payload_size = payload.size() * sizeof(int16);

  audio_decoder_->IncomingParsedRtpPacket(payload_data, payload_size,
                                          rtp_header);


  int number_of_10ms_blocks = 2;
  int desired_frequency = 16000;
  PcmAudioFrame audio_frame;
  uint32 rtp_timestamp;

  EXPECT_TRUE(audio_decoder_->GetRawAudioFrame(number_of_10ms_blocks,
                                               desired_frequency,
                                               &audio_frame,
                                               &rtp_timestamp));

  EXPECT_EQ(2, audio_frame.channels);
  EXPECT_EQ(16000, audio_frame.frequency);
  EXPECT_EQ(640ul, audio_frame.samples.size());
  for (size_t i = 10 * audio_config.channels; i < audio_frame.samples.size();
      ++i) {
    EXPECT_EQ(0x3412, audio_frame.samples[i]);
  }

  rtp_header.webrtc.header.sequenceNumber++;
  rtp_header.webrtc.header.timestamp += (audio_config.frequency / 100) * 2 * 2;
  audio_decoder_->IncomingParsedRtpPacket(payload_data, payload_size,
                                          rtp_header);

  EXPECT_EQ(2, audio_frame.channels);
  EXPECT_EQ(16000, audio_frame.frequency);
  EXPECT_EQ(640ul, audio_frame.samples.size());
  // First 10 samples per channel are 0 from NetEq.
  for (size_t i = 10 * audio_config.channels; i < audio_frame.samples.size();
      ++i) {
    EXPECT_EQ(0x3412, audio_frame.samples[i]);
  }
  run_loop.RunUntilIdle();
}

TEST_F(AudioDecoderTest, Pcm16Resample) {
  base::RunLoop run_loop;
  AudioReceiverConfig audio_config;
  audio_config.rtp_payload_type = 127;
  audio_config.frequency = 16000;
  audio_config.channels = 2;
  audio_config.codec = kPcm16;
  audio_config.use_external_decoder = false;
  Configure(audio_config);

  RtpCastHeader rtp_header;
  rtp_header.webrtc.header.payloadType = 127;
  rtp_header.webrtc.header.sequenceNumber = 1234;
  rtp_header.webrtc.header.timestamp = 0x87654321;
  rtp_header.webrtc.header.ssrc = 0x12345678;
  rtp_header.webrtc.header.paddingLength = 0;
  rtp_header.webrtc.header.headerLength = 12;

  rtp_header.webrtc.type.Audio.isCNG = false;
  rtp_header.webrtc.type.Audio.channel = 2;

  std::vector<int16> payload(640, 0x1234);

  uint8* payload_data = reinterpret_cast<uint8*>(&payload[0]);
  int payload_size = payload.size() * sizeof(int16);

  audio_decoder_->IncomingParsedRtpPacket(payload_data, payload_size,
                                          rtp_header);

  int number_of_10ms_blocks = 2;
  int desired_frequency = 48000;
  PcmAudioFrame audio_frame;
  uint32 rtp_timestamp;

  EXPECT_TRUE(audio_decoder_->GetRawAudioFrame(number_of_10ms_blocks,
                                               desired_frequency,
                                               &audio_frame,
                                               &rtp_timestamp));

  EXPECT_EQ(2, audio_frame.channels);
  EXPECT_EQ(48000, audio_frame.frequency);
  EXPECT_EQ(1920ul, audio_frame.samples.size());  // Upsampled to 48 KHz.
  int count = 0;
  // Resampling makes the variance worse.
  for (size_t i = 100 * audio_config.channels; i < audio_frame.samples.size();
      ++i) {
    EXPECT_NEAR(0x3412, audio_frame.samples[i], 400);
    if (0x3412 == audio_frame.samples[i])  count++;
  }
  run_loop.RunUntilIdle();
}

}  // namespace cast
}  // namespace media