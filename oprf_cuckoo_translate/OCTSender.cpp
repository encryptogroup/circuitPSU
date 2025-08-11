#include "OCTSender.h"
#include "SimpleIndexParameters.h"
#include "cryptoTools/Common/CLP.h"
#include "utils.h"
#include "volePSI/GMW/Gmw.h"
#include <ShareTranslationCircuit.h>
#include <fstream>

using namespace std;
using namespace oc;


// THIS IS P1 in our protocol

void report_networking(coproto::Socket* chl, OCTSender *p, std::string what) {
  size_t sent = 0, recv = 0;

  sent += chl->bytesSent();
  recv += chl->bytesReceived();

  std::cout << std::endl
            << "---------------------------------------" << std::endl;
  std::cout << "Networking for:\t" << what << std::endl;
  std::cout << "Total recv:\t" << recv / 1024.0 / 1024.0 << "MB\tTotal sent:\t"
            << sent / 1024.0 / 1024.0 << "MB\t"
            << "TOTAL:\t" << (recv + sent) / 1024.0 / 1024.0 << "MB"
            << std::endl;

  std::cout << "Diff recv:\t" << (recv - p->last_received) / 1024.0 / 1024.0
            << "MB\tDiff sent:\t" << (sent - p->last_sent) / 1024.0 / 1024.0
            << "MB\t"
            << "DIFF:\t"
            << (recv + sent - (p->last_received + p->last_sent)) / 1024.0 /
                   1024.0
            << "MB";
  std::cout << std::endl
            << "---------------------------------------" << std::endl;

  p->last_sent = sent;
  p->last_received = recv;
}

void OCTSender::runMPOPRF(oc::Socket *chls, size_t width,
                          size_t hash_length_in_bytes) {
  std::cout << "Starting MPOPRF\n";
	oc::PRNG prng(oc::toBlock(123));
  mp_oprf_sender.setParams(toBlock(123456), larger_set_size,
                           log2ceil(larger_set_size), width, hash_length_in_bytes,
                           32, 1 << 8, 1 << 8);

  mp_oprf_sender.run(prng, chls, context.num_threads);
  report_networking(chls, this, "MPOPRF");
  std::cout << "Finished MPOPRF\n";
}

oc::BitVector OCTSender::runCPSI(oc::Socket *chls,
                                 std::vector<oc::block> &recvSet, oc::u64 nt) {
  volePSI::RsCpsiReceiver::Sharing rShare;
  std::cout << "Starting CPSI\n";
  auto p0 = rs_cpsi_receiver.receive(recvSet, rShare, chls[0]);

  eval(p0);
  oc::BitVector intersectionShare(rShare.mFlagBits.size());
  for (int i = 0; i < rShare.mFlagBits.size(); i++) {
    bool bitShare = rShare.mFlagBits[i];
    // this is currently not an ideal solution but may be ammended
    // get index of element in sender set that is transmitted by this hashmask
    // bit may be empty: use this indicator, to prevent sending something
    elementMapping.push_back(-1);
    intersectionShare[i] = bitShare;
  }
  for (int i = 0; i < context.sender_size; i++) {
    // the element of the sender set at position i is at position k in the
    // bitvector of the intersection
    auto k = rShare.mMapping[i];
    elementMapping[k] = i;
  }

  report_networking(chls, this, "CPSI");
  std::cout << "Finished CPSI\n";
  return intersectionShare;
}

// to use this mode of share translation, need to reset to old permutation style
// change pi to be a vector of ints
// toggle the switch in runCNP

oc::BitVector OCTSender::runSTCBit(long n, oc::Socket *chls) {

  oc::BitVector Delta;

  oc::BitVector prog = oc::ShareTranslationCircuit::GenerateProgramming(&(pi->mPi));

  // Run GMW
  Delta = bitGMW->run(n, 1, &prog, 1, &shareTranslationCircuit, chls);
  return Delta;
}


#include <random>

void OCTSender::runCNP(oc::BitVector x0, oc::Socket *chls) {
  std::cout << "Starting CNP\n";

  int n = x0.size();
  // set permutation
	oc::PRNG prng(oc::toBlock(123));
  pi = new secJoin::Perm(n,prng);

  if(permutationNetworkBased)
  {

  // TODO run share translate
  oc::BitVector Delta = runSTCBit(x0.size(), chls);

  // m
  oc::BitVector m(x0.size());
  // receive m = x1 ^ a
  cp::sync_wait(chls[0].recv(m));

  oc::BitVector perm_input(x0.size());
  // compute perm_input = m ^ x0
  perm_input = m ^ x0;

  oc::BitVector pi_input(x0.size());
  oc::BitVector mobf(x0.size());
  // compute ~m = pi(perm_input) ^ Delta
  for (int i = 0; i < x0.size(); i++) {
    pi_input[i] = perm_input[pi->mPi[i]];
  }

  mobf = pi_input ^ Delta;

  // send ~m
  cp::sync_wait(chls[0].send(mobf));

  }else{
    // do correlation generator of https://eprint.iacr.org/2024/547

    int rowSize = 1;
    int b = 16;


    

    oc::Matrix<u8> x0m(n, rowSize);
    //insert x0 into x0m
    for(int i = 0;i<n;i++)
    {
      x0m(i,0) = (int) x0[i];
    }


    oc::Matrix<u8> sout0(n, rowSize);

    // Prepare correlation


		secJoin::AltModPermGenSender AltModPerm0;
    secJoin::CorGenerator ole0;

    coproto::Socket aChannel = chls[0].fork();
    coproto::Socket bChannel = chls[0].fork();


    ole0.init(std::move(bChannel), prng, 0, 1, 1 << b, 0);

		AltModPerm0.init(n, rowSize, ole0);

		secJoin::PermCorSender perm0;


    auto r = macoro::sync_wait(macoro::when_all_ready(
			ole0.start(),
			AltModPerm0.generate(*pi, prng, aChannel, perm0)
		));

		std::get<0>(r).result();

    // compute permutation from correlation

    cp::sync_wait(perm0.apply<u8>(secJoin::PermOp::Regular,x0m,sout0,chls[0]));
    
    oc::BitVector y0(x0.size());
    // put sout0 into y0
    for(int i = 0;i<n;i++)
    {
      y0[i] = (bool) ( sout0(i,0) );
    }
    cp::sync_wait(chls[0].send(y0));
  }
  report_networking(chls, this, "CNP");

  std::cout << "Finished CNP\n";
}

void OCTSender::setSenderSet(const std::vector<oc::block> &sender_set,
                             size_t receiver_size) {
  this->sender_set = sender_set;
}

void OCTSender::setup(oc::Socket *chls) {
  
	oc::PRNG prng(oc::toBlock(123));
  larger_set_size = std::max(context.sender_size,context.receiver_size);
  smaller_set_size = std::min(context.sender_size,context.receiver_size);
  // auto byteLength = sizeof(block);
  // Last param: number of threads
  rs_cpsi_receiver.init(context.receiver_size, context.sender_size, 0, 40,
                        prng.get(), context.num_threads);
  report_networking(chls, this, "setupCPSI");
  timer->setTimePoint("after setupCPSI");
  //bitGMW = new oc::BitGMW();

  auto params = oc::CuckooIndex<>::selectParams(context.sender_size, 40, 0, 3);
  auto numBins = params.numBins();
  if(permutationNetworkBased){
    bitGMW = new oc::BitGMW();
    oc::ShareTranslationCircuit::GenerateCircuit(numBins, &shareTranslationCircuit);
    bitGMW->setup(1, &shareTranslationCircuit, chls);
  }

  report_networking(chls, this, "setupCNP");
  timer->setTimePoint("after setupCNP");

  sendMessages.resize(numBins);
  elementSender.configure(numBins, 2, context.num_threads,
                          SilentSecType::SemiHonest);
  cp::sync_wait(elementSender.genSilentBaseOts(prng, chls[0], true));
  cp::sync_wait(elementSender.send(sendMessages, prng, chls[0]));
  report_networking(chls, this, "setupOT");
  timer->setTimePoint("after setupOT");
  report_networking(chls, this, "setup");
}
void OCTSender::output(oc::Socket *chls) {
  auto params = getMpOprfParams(0, smaller_set_size, larger_set_size);
  runMPOPRF(chls, params.first, params.second);
  vector<u8> oprf_values = mp_oprf_sender.get_oprf(sender_set);
  timer->setTimePoint("after runMpOprf");
  std::vector<oc::block> senderSet;
  auto hashLengthInBytes = params.second;
  for (int i = 0; i < context.sender_size; i++) {
    oc::u64 up = 0;
    oc::u64 down = 0;
    for (int j = 0; j < hashLengthInBytes; j++) {
      if (j < 8) {
        down += oprf_values[i * hashLengthInBytes + j] << ((j % 8) * 8);
      } else {
        up += oprf_values[i * hashLengthInBytes + j] << ((j % 8) * 8);
      }
    }
    senderSet.push_back(oc::toBlock(up, down));
  }
  auto psiVectorShares = runCPSI(chls, senderSet);
  timer->setTimePoint("after runCPSI");

  runCNP(psiVectorShares, chls);
  timer->setTimePoint("after runCNP");
  std::cout << "Starting FinalOT\n";
  oc::BitVector d(elementMapping.size());
  cp::sync_wait(chls[0].recv(d));
  // pack the send messages
  block bot = oc::toBlock(-1, -1);

  std::vector<std::array<block, 2>> messages(elementMapping.size());
  for (int i = 0; i < elementMapping.size(); i++) {
    // which element is at point i in the inclusion vector
    int index = elementMapping[pi->mPi[i]];
    // std::cout << i << " -> " << index;
    if (index == -1) {
      // there is no element (this is the case if we have collision avoidance
      // from the rcpsi)
      messages[i][0] = bot;
      // messages[i][1] = bot;
      // std::cout << " : There was no element here, set \n";
    } else {
      // there is an actual element here, that needs to be packed
      messages[i][0] = sender_set[index];
      // messages[i][1] = bot;
      //  std::cout << " : There was " << sender_set[index] <<"\n";
    }
  }

  // run sending
  std::vector<oc::block> tempMessages(elementMapping.size());
  for (int i = 0; i < elementMapping.size(); i++) {
    bool d_c = d[i];
    tempMessages[i] = sendMessages[i][d_c] ^ messages[i][0];
    // tempMessages[i][1] = sendMessages[i][d_c^1] ^ messages[i][1];
  }
  cp::sync_wait(chls[0].send(tempMessages));
  report_networking(chls, this, "FinalOT");
  std::cout << "Finished FinalOT\n";
  timer->setTimePoint("after runFinalOT");
}
void OCTSender::setTimer(oc::Timer &timer) {
  this->timer = &timer;
  mp_oprf_sender.setTimer(timer);
}
