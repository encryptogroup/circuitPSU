#pragma once

#include "MPOPRFSender.h"
#include "cryptoTools/Common/Timer.h"
#include "macoro/sync_wait.h"
#include "macoro/task.h"
#include "macoro/when_all.h"
#include "volePSI/RsCpsi.h"
#include "volePSI/RsPsi.h"
#include "secure-join/Perm/Permutation.h"
#include "secure-join/Perm/PermCorrelation.h"
#include "secure-join/CorGenerator/CorGenerator.h"
#include "secure-join/Perm/AltModPerm.h"
#include "secure-join/Util/Util.h"
#include <BitGMW.h>
#include <PSUSender.h>
#include <map>

//#include <libOTe/NChooseOne/Kkrt/KkrtNcoOtReceiver.h>
//#include "libOTe/NChooseOne/Oos/OosNcoOtReceiver.h"
//#include "libOTe/NChooseOne/Oos/OosNcoOtSender.h"

// THIS IS P1 in our protocol
class OCTSender : public PSUSender {

  inline auto eval(macoro::task<> &t0) {
    auto r = macoro::sync_wait(macoro::when_all_ready(std::move(t0)));
    std::get<0>(r).result();
  }

  bool permutationNetworkBased = true;
  size_t larger_set_size;
  size_t smaller_set_size;
  oc::MPOPRFSender mp_oprf_sender;
  volePSI::RsCpsiReceiver rs_cpsi_receiver;

  oc::BitGMW *bitGMW;
  oc::BetaCircuit shareTranslationCircuit;
  secJoin::Perm* pi;
  std::vector<int> elementMapping;
  oc::SilentOtExtSender elementSender;
  std::vector<std::array<oc::block, 2>> sendMessages;


  void runMPOPRF(oc::Socket *chls, size_t width, size_t hash_length_in_bytes);

  oc::BitVector
  runCPSI(oc::Socket *chls, std::vector<oc::block> &sendSet, oc::u64 nt = 1);

public:
  size_t last_sent;
  size_t last_received;
  void setSenderSet(const std::vector<oc::block> &sender_set,
                    size_t receiver_size);
  void setup(coproto::Socket *chls);
  void output(oc::Socket *chls);
  void setTimer(oc::Timer &timer);
  // oc::BitVector runST(int n, oc::Socket* chls);
  // oc::BitVector runSTC(int n, oc::Socket* chls);
  oc::BitVector runSTCBit(long n, oc::Socket *chls);

  void runCNP(oc::BitVector x0, oc::Socket *chls);
};
