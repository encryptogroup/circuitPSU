#pragma once

#include "MPOPRFReceiver.h"
#include "cryptoTools/Common/Timer.h"
#include "macoro/sync_wait.h"
#include "macoro/task.h"
#include "macoro/when_all.h"
#include "volePSI/RsCpsi.h"
#include "volePSI/RsPsi.h"
#include "secure-join/CorGenerator/CorGenerator.h"
#include "secure-join/Perm/PermCorrelation.h"
#include "secure-join/Perm/AltModPerm.h"
#include "secure-join/Util/Util.h"
#include <BitGMW.h>
#include <PSUReceiver.h>
#include <libOTe/TwoChooseOne/Iknp/IknpOtExtReceiver.h>
//#include <libOTe/NChooseOne/Kkrt/KkrtNcoOtSender.h>
//#include "libOTe/NChooseOne/Oos/OosNcoOtReceiver.h"
//#include "libOTe/NChooseOne/Oos/OosNcoOtSender.h"

// THIS IS P2 in our protocol
class OCTReceiver : public PSUReceiver {
  inline auto eval(macoro::task<> &t0) {
    auto r = macoro::sync_wait(macoro::when_all_ready(std::move(t0)));
    std::get<0>(r).result();
  }
  bool permutationNetworkBased = true;
  size_t larger_set_size;
  size_t smaller_set_size; 
  oc::MPOPRFReceiver mp_oprf_receiver;
  volePSI::RsCpsiSender rs_cpsi_sender;
  oc::SilentOtExtReceiver elementReceiver;
  oc::BitGMW *bitGMW;
  std::vector<oc::block> receivedMessages;
  oc::BitVector preChoices;
  oc::BetaCircuit shareTranslationCircuit;

  oc::u8 *runMpOprf(oc::Socket *chls, const std::vector<oc::block> &x_set,
                    size_t width, size_t hash_length_in_bytes);

public:
  void setReceiverSet(const std::vector<oc::block> &receiver_set,
                      size_t sender_size);
  oc::BitVector
  runCPSI(oc::Socket *chls, std::vector<oc::block> &recvSet, oc::u64 nt = 1);

  void setup(coproto::Socket *chls);
  std::vector<oc::block> output(oc::Socket *chls);
  void setTimer(oc::Timer &timer);

  // std::tuple<oc::BitVector,oc::BitVector> runST(int n,oc::Socket* chls);
  // std::tuple<oc::BitVector,oc::BitVector> runSTC(int n,oc::Socket* chls);
  std::tuple<oc::BitVector, oc::BitVector> runSTCBit(long n, oc::Socket *chls);

  oc::BitVector runCNP(oc::BitVector x1, oc::Socket *chls);
};
