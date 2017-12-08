/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#pragma once

#include <stdint.h>

namespace mozquic {

class BufferedPacket
{
  // todo a version that creates this in one allocation
public:

  BufferedPacket(const unsigned char *pkt, uint32_t pktSize, uint32_t headerSize,
                 uint64_t packetNum)
    : mData(new unsigned char[pktSize])
    , mLen(pktSize)
    , mHeaderSize(headerSize)
    , mPacketNum(packetNum)
    , mExplicitPeer(false)
    , mBareAck(false)
  {
    memcpy((void *)mData.get(), pkt, mLen);
  }

  BufferedPacket(const unsigned char *pkt, uint32_t pktSize,
                 struct sockaddr_in *sin, uint64_t packetNum,
                 bool bareAck)
    : mData(new unsigned char[pktSize])
    , mLen(pktSize)
    , mHeaderSize(0)
    , mPacketNum(packetNum)
    , mExplicitPeer(false)
    , mBareAck(bareAck)
  {
    memcpy((void *)mData.get(), pkt, mLen);
    if (sin) {
      mExplicitPeer = true;
      memcpy(&mSockAddr, sin, sizeof (struct sockaddr_in));
    }
  }
  ~BufferedPacket()
  {
  }

  std::unique_ptr<const unsigned char []>mData;
  uint32_t mLen;
  uint32_t mHeaderSize;
  uint32_t mPacketNum;
  bool     mExplicitPeer;
  bool     mBareAck;
  struct sockaddr_in mSockAddr;
};

const uint32_t kDefaultMSS = 1460;
const uint32_t kMinWindow = 2 * kDefaultMSS;

class Sender final
{
public:
  Sender(MozQuic *session);
  uint32_t Transmit(uint64_t packetNumber, bool bareAck,
                    const unsigned char *, uint32_t len, struct sockaddr_in *peer);
  void RTTSample(uint64_t xmit, uint16_t delay);
  void Ack(uint64_t packetNumber, uint32_t packetLength);
  void ReportLoss(uint64_t packetNumber, uint32_t packetLength);
  void SetDropRate(uint64_t dr) { mDropRate = dr; }
  uint16_t DropRate() { return mDropRate; }
  uint32_t Tick(const uint64_t now);
  void Connected();
  bool CanSendNow(uint64_t amt);
  uint16_t SmoothedRTT() { return mSmoothedRTT; }
  
  bool EmptyQueue() 
  {
    return mQueue.empty();
  }

private:
  
  MozQuic *mMozQuic;
  std::list<std::unique_ptr<BufferedPacket>> mQueue;
  uint16_t mSmoothedRTT;
  uint16_t mDropRate;

  bool mCCState;
  uint64_t mPacingTicker;

  uint64_t mWindow; // bytes
  uint64_t mWindowUsed; // bytes

  uint64_t mUnPacedPacketCredits;
  uint64_t mLastSend;
  uint64_t mSSThresh;
  uint64_t mEndOfRecovery;
};

} //namespace