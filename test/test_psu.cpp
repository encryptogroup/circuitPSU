#include <iostream>

#include <cryptoTools/Common/Defines.h>
#include <cryptoTools/Common/Timer.h>
#include <cryptoTools/Network/Channel.h>
#include <cryptoTools/Network/IOService.h>
#include <cryptoTools/Network/Session.h>

#include "OCTReceiver.h"
#include "OCTSender.h"
#include "PSUReceiver.h"
#include "PSUSender.h"
#include "coproto/Socket/AsioSocket.h"
#include "cryptoTools/Network/IOService.h"

#include "utils.h"
#include <cmdline.h>
#include <sys/resource.h>
using namespace osuCrypto;
using namespace std;

void run_sender(const Context &context) {
  auto chl =
      cp::asioConnect(context.host + ":" + std::to_string(context.port), true);

  PRNG prng(toBlock(123));
  vector<block> senderSet(context.sender_size);
  for (auto i = 0; i < context.sender_size; ++i) {
    senderSet[i] = toBlock(i + 1);
  }

  /*
  for (auto i = 0; i < context.receiver_size; ++i) {
    senderSet[i] = prng.get<block>();
  }
  */
  Socket chls[context.num_threads];
  chls[0] = (chl);
  for (int i = 1; i < context.num_threads; i++) {
    chls[i] = (chl.fork());
  }

  PSUSender *sender = new OCTSender;

  sender->setContext(context);

  sender->setSenderSet(senderSet, context.receiver_size);
  Timer timer;
  sender->setTimer(timer);
  timer.reset();
  timer.setTimePoint("before output");
  sender->setup(chls);
  timer.setTimePoint("after setup");
  sender->output(chls);
  timer.setTimePoint("after output");

  cout << IoStream::lock;
  cout << "Sender: " << typeid(*sender).name() << endl;
  cout << timer << endl;
  // report_networking(chls[0]);
  cout << IoStream::unlock;
  delete sender;
}

void run_receiver(const Context &context) {

  auto chl =
      cp::asioConnect(context.host + ":" + std::to_string(context.port), false);

  vector<block> receiverSet(context.receiver_size);
  PRNG prng(toBlock(123));

  Socket chls[context.num_threads];
  chls[0] = (chl);
  for (int i = 1; i < context.num_threads; i++) {
    chls[i] = (chl.fork());
  }

  int difference = 5;

  for (auto i = 0; i < context.receiver_size; ++i) {
    receiverSet[i] = toBlock(i + difference + 1);
  }
  /*
  for (auto i = 0; i < context.receiver_size; ++i) {
    receiverSet[i] = prng.get<block>();
  }
  */
  PSUReceiver *receiver = new OCTReceiver;

  receiver->setContext(context);

  receiver->setReceiverSet(receiverSet, context.sender_size);

  Timer timer;
  receiver->setTimer(timer);
  timer.reset();
  timer.setTimePoint("before output");
  receiver->setup(chls);
  timer.setTimePoint("after setup");
  auto res = receiver->output(chls);
  timer.setTimePoint("after output");

  cout << IoStream::lock;
  cout << "Receiver: " << typeid(*receiver).name() << endl;
  cout << "Union size: " << res.size() << endl;
  cout << timer << endl;

  cout << IoStream::unlock;
  std::cout << " Checking for duplicates...\n";
  delete receiver;
  // Correctness check
  // check for no duplicates
  bool duplicates = false;
  for (int i = 0; i < res.size(); i++) {

    for (int j = i + 1; j < res.size(); j++) {
      if (res[i] == res[j]) {
        duplicates = true;
        std::cout << "Found duplicate: " << res[i] << " at " << i << " and "
                  << j << "\n";
        break;
      }
    }
    if (duplicates) {
      break;
    }
  }
  if (!duplicates) {
    std::cout << "No duplicates found!\n";
  }
}

Context parse_arguments(int argc, char **argv) {
  cmdline::parser parser;

  parser.add<size_t>("role", 'u', "role(0: unit, 1: sender, 2: receiver)",
                     false, 0);
  parser.add<string>("host", '\0', "host name", false, "127.0.0.1");
  parser.add<size_t>("port", '\0', "port number", false, 12345);
  parser.add<size_t>("sender", 's', "sender set size (log2)", false, 8);
  parser.add<size_t>("receiver", 'r', "receiver set size (log2)", false, 8);
  parser.add<size_t>("threads", 't', "threads", false, 1);

  parser.parse_check(argc, argv);

  Context context;
  context.role = parser.get<size_t>("role");
  context.host = parser.get<string>("host");
  context.port = parser.get<size_t>("port");
  context.sender_size = 1ull << parser.get<size_t>("sender");
  context.receiver_size = 1ull << parser.get<size_t>("receiver");
  context.num_threads = parser.get<size_t>("threads");

  return context;
}

int main(int argc, char **argv) {
  Context context = parse_arguments(argc, argv);

  const rlim_t kStackSize = 128 * 1024 * 1024; // min stack size = 32 MB
  struct rlimit rl;
  int result;

  result = getrlimit(RLIMIT_STACK, &rl);
  if (result == 0) {
    if (rl.rlim_cur < kStackSize) {
      rl.rlim_cur = kStackSize;
      result = setrlimit(RLIMIT_STACK, &rl);
      if (result != 0) {
        fprintf(stderr, "setrlimit returned result = %d\n", result);
      }
    }
  }
  cout << "===arguments==="
       << "\nrole:" << context.role << "\nsender_size:" << context.sender_size
       << "\nreceiver_size:" << context.receiver_size
       << "\nnum_threads:" << context.num_threads << "\n===arguments===\n\n";

  if (context.role == 0) {
    auto recver_thrd = std::thread(run_receiver, context);
    auto sender_thrd = std::thread(run_sender, context);
    recver_thrd.join();
    sender_thrd.join();
  } else if (context.role == 1) {
    run_sender(context);
  } else if (context.role == 2) {
    run_receiver(context);
  }

  return 0;
}
