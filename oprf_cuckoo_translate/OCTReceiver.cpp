#include "OCTReceiver.h"
#include "SimpleIndexParameters.h"
#include "coproto/Socket/BufferingSocket.h"
#include "cryptoTools/Common/CLP.h"
#include "utils.h"
#include "volePSI/GMW/Gmw.h"
#include <ShareTranslationCircuit.h>
#include <fstream>
#include <set>

using namespace oc;
using namespace std;
// THIS IS P2 in our protocol

oc::u8 *OCTReceiver::runMpOprf(Socket *chls,
                               const std::vector<oc::block> &x_set,
                               size_t width, size_t hash_length_in_bytes) {
  std::cout << "Starting MPOPRF\n";
  
	oc::PRNG prng(oc::toBlock(123));
  mp_oprf_receiver.setParams(toBlock(123456), larger_set_size,
                             log2ceil(larger_set_size), width,
                             hash_length_in_bytes, 32, 1 << 8, 1 << 8);
  auto result = mp_oprf_receiver.run(prng, chls, x_set, context.num_threads);

  std::cout << "Finished MPOPRF\n";
  return result;
}

oc::BitVector OCTReceiver::runCPSI(oc::Socket *chls,
                                   std::vector<oc::block> &sendSet, oc::u64 nt) {

  std::cout << "Starting CPSI\n";
  // oc::Matrix<u8> senderValues(sendSet.size(), sizeof(block));
  // std::memcpy(senderValues.data(), sendSet.data(),sendSet.size() *
  // sizeof(block));
  volePSI::RsCpsiSender::Sharing sShare;
  auto p1 = rs_cpsi_sender.send(sendSet, {}, sShare, chls[0]);

  eval(p1);

  oc::BitVector intersectionShare(sShare.mFlagBits.size());
  for (int i = 0; i < sShare.mFlagBits.size(); ++i) {
    bool bitShare = sShare.mFlagBits[i];
    intersectionShare[i] = bitShare;
  }

  std::cout << "Finished CPSI\n";
  return intersectionShare;
}

std::tuple<oc::BitVector, oc::BitVector> OCTReceiver::runSTCBit(long n, oc::Socket *chls) {

  // TODO uniform n bit string
  oc::BitVector a = oc::BitVector(n);
  oc::BitVector b = oc::BitVector(n);

  // Run GMW

  // Get this from the circuit
  oc::BitVector input = oc::BitVector(a);
  input.append(b);
  bitGMW->run(n, 0, &input, 0, &shareTranslationCircuit, chls);
  return {a, b};
}



oc::BitVector OCTReceiver::runCNP(oc::BitVector x1, oc::Socket *chls) {

  std::cout << "Starting CNP\n";

	oc::PRNG prng(oc::toBlock(123));
  // CNP protocol needs to be set for the size of the hashed intersection set
  // Need to assign each bit to the resulting value
  oc::BitVector x_perm;
  if(permutationNetworkBased)
  {

  // run share translate
  auto result = runSTCBit(x1.size(), chls);
  oc::BitVector a = std::get<0>(result);
  oc::BitVector b = std::get<1>(result);



  // compute m = a ^ x1
  oc::BitVector m = a ^ x1;
  // send m
  cp::sync_wait(chls[0].send(m));
  oc::BitVector message_perm(x1.size());
  // receive ~m
  cp::sync_wait(chls[0].recv(message_perm));

  // compute ~x = b ^ ~m
  x_perm = b ^ message_perm;

  }else{
    // do correlation generator of https://eprint.iacr.org/2024/547
    int rowSize = 1;
    int n = x1.size();

    int b = 16;

    



    oc::Matrix<u8> x1m(n, rowSize);
    // insert x1 into x1m
    for(int i = 0;i<n;i++)
    {
      x1m(i,0) = (int) x1[i];
    }
    oc::Matrix<u8> sout1(n, rowSize);

    

    // Prepare correlation

		secJoin::AltModPermGenReceiver AltModPerm1;
    secJoin::CorGenerator ole1;
    
    coproto::Socket aChannel = chls[0].fork();
    coproto::Socket bChannel = chls[0].fork();

    ole1.init(std::move(bChannel), prng, 1, 1, 1 << b, 0);

		AltModPerm1.init(n, rowSize, ole1);

		secJoin::PermCorReceiver perm1;


    auto r = macoro::sync_wait(macoro::when_all_ready(
			ole1.start() ,
			AltModPerm1.generate(prng, aChannel, perm1)
		));

		std::get<0>(r).result();

    // compute permutation from correlation

    cp::sync_wait(perm1.apply<u8>(secJoin::PermOp::Regular,x1m,sout1,chls[0]));

    oc::BitVector y0(x1.size());

    oc::BitVector y1(x1.size());
    // put sout1 into y1
    for(int i = 0;i<n;i++)
    {
      y1[i] = (bool) ( sout1(i,0) );
    }
    cp::sync_wait(chls[0].recv(y0));
    x_perm = y0 ^ y1;
  }
  std::cout << "Finished CNP\n";
  return x_perm;
}



void OCTReceiver::setReceiverSet(const std::vector<oc::block> &receiver_set,
                                 size_t sender_size) {
  this->receiver_set = receiver_set;
  // Last param, number of threads
}

void OCTReceiver::setup(oc::Socket *chls) {
  
	oc::PRNG prng(oc::toBlock(123));
  larger_set_size = std::max(context.sender_size,context.receiver_size);
  smaller_set_size = std::min(context.sender_size,context.receiver_size);
  // auto byteLength = sizeof(block);
  rs_cpsi_sender.init(context.receiver_size, context.sender_size, 0, 40,
                      prng.get(), context.num_threads);

  timer->setTimePoint("after setupCPSI");
  auto params = oc::CuckooIndex<>::selectParams(context.sender_size, 40, 0, 3);
  auto numBins = params.numBins();
  if(permutationNetworkBased)
  {
    bitGMW = new oc::BitGMW();
    oc::ShareTranslationCircuit::GenerateCircuit(numBins, &shareTranslationCircuit);
    bitGMW->setup(0, &shareTranslationCircuit, chls);
  }

  timer->setTimePoint("after setupCNP");

  elementReceiver.configure(numBins, 2, context.num_threads,
                            SilentSecType::SemiHonest);
  cp::sync_wait(elementReceiver.genSilentBaseOts(prng, chls[0], true));

  // preChoices = b
  preChoices.resize(numBins);
  preChoices.randomize(prng);
  // receivedMessages = xb
  receivedMessages.resize(numBins);
  cp::sync_wait(
      elementReceiver.receive(preChoices, receivedMessages, prng, chls[0]));
  timer->setTimePoint("after setupOT");
}

std::vector<oc::block> OCTReceiver::output(Socket *chls) {
  auto paramsOPRF = getMpOprfParams(0, smaller_set_size, larger_set_size);
  size_t hashLengthInBytes = paramsOPRF.second;

  auto shares = this->receiver_set;
  u8 *oprfs = runMpOprf(chls, shares, paramsOPRF.first, paramsOPRF.second);
  timer->setTimePoint("after runMpOprf");
  std::vector<oc::block> receiverSet;
  for (int i = 0; i < context.receiver_size; i++) {
    oc::u64 up = 0;
    oc::u64 down = 0;
    for (int j = 0; j < hashLengthInBytes; j++) {
      if (j < 8) {
        down += oprfs[i * hashLengthInBytes + j] << ((j % 8) * 8);
        ;
      } else {
        up += oprfs[i * hashLengthInBytes + j] << ((j % 8) * 8);
        ;
      }
    }
    receiverSet.push_back(oc::toBlock(up, down));
  }
  auto psiVectorShares = runCPSI(chls, receiverSet);
  timer->setTimePoint("after runCPSI");
  auto choices = runCNP(psiVectorShares, chls);
  timer->setTimePoint("after runCNP");
  std::cout << "Starting FinalOT\n";

  oc::BitVector d = preChoices ^ choices;
  cp::sync_wait(chls[0].send(d));
  std::vector<oc::block> tempReceivedMessages(choices.size());
  cp::sync_wait(chls[0].recv(tempReceivedMessages));
  std::cout << "Finished FinalOT\n";
  timer->setTimePoint("after runOT");

  vector<block> union_result = this->receiver_set;
  for(const auto block : union_result)
  {
    std::cout << "Union contains " << block << std::endl;
  }
  block botElement = oc::toBlock(-1, -1);

  for (int i = 0; i < choices.size(); i++) {
    if (choices[i] == 0) {
      block message = tempReceivedMessages[i] ^ receivedMessages[i];
      // need to filter out bot because of longer cpsi vector
      if (message != botElement) {
        std::cout << "Adding to union " << message << std::endl;
        union_result.push_back(message);
      }
    }
  }
  return union_result;
}
void OCTReceiver::setTimer(oc::Timer &timer) {
  mp_oprf_receiver.setTimer(timer);
  this->timer = &timer;
}
