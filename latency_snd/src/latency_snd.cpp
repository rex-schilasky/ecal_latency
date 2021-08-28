#include <ecal/ecal.h>

#include <chrono>
#include <iostream>

#include <tclap/CmdLine.h>

// warmup runs not to measure
const int warmups(10);

// time getter
long long get_microseconds()
{
  std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
  return(std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch()).count());
}

// single test run
void do_run(const int runs, int snd_size /*kB*/, int delay /*ms*/, int mem_buffer, bool zero_copy)
{
  // log parameter
  std::cout << "--------------------------------------------"    << std::endl;
  std::cout << "Runs                    : " << runs              << std::endl;
  std::cout << "Message size            : " << snd_size << " kB" << std::endl;
  std::cout << "Message delay           : " << delay    << " ms" << std::endl;
  std::cout << "Memory buffer           : " << mem_buffer        << std::endl;
  if (zero_copy)
  {
    std::cout << "Zero copy               : ON"  << std::endl;
  }
  else
  {
    std::cout << "Zero copy               : OFF" << std::endl;
  }

  // initialize eCAL API
  eCAL::Initialize(0, nullptr, "latency_snd");

  // create publisher and subscriber
  eCAL::CPublisher pub("ping");

  // set number of publisher memory buffers
  pub.SetBufferCount(mem_buffer);

  // enable zero copy mode
  pub.EnableZeroCopy(zero_copy);

  // prepare send buffer
  std::vector<char> snd_array(snd_size * 1024);

  // let them match
  eCAL::Process::SleepMS(2000);

  // add some extra loops for warmup :-)
  int run(0);
  for (run = 0; run < runs+warmups; ++run)
  {
    // get time and send message
    pub.Send(snd_array.data(), snd_array.size(), get_microseconds());
    // delay
    eCAL::Process::SleepMS(delay);
  }

  // log test
  std::cout << "Messages sent           : " << run - warmups << std::endl;
  std::cout << "--------------------------------------------" << std::endl;

  // let the receiver do the evaluation
  eCAL::Process::SleepMS(2000);

  // finalize eCAL API
  eCAL::Finalize();
}

int main(int argc, char **argv)
{
  try
  {
    // parse command line
    TCLAP::CmdLine cmd("latency_snd");
    TCLAP::ValueArg<int>         runs      ("r", "runs",       "Number of messages to send.",            false, 1000, "int");
    TCLAP::ValueArg<int>         size      ("s", "size",       "Messages size in kB.",                   false,   -1, "int");
    TCLAP::ValueArg<int>         delay     ("d", "delay",      "Messages send delay in ms.",             false,   50, "int");
    TCLAP::ValueArg<int>         mem_buffer("b", "mem_buffer", "Number of memory files per connection.", false,    1, "int");
    TCLAP::SwitchArg             zero_copy ("z", "zero_copy",  "Switch zero copy mode on.");
    cmd.add(runs);
    cmd.add(size);
    cmd.add(delay);
    cmd.add(mem_buffer);
    cmd.add(zero_copy);
    cmd.parse(argc, argv);

    if(size < 0)
    {
      // automatic size mode
      for (int s = 1; s <= 16384; s *= 2) do_run(runs.getValue(), s, delay.getValue(), mem_buffer.getValue(), zero_copy.getValue());
    }
    else
    {
      // run single test
      do_run(runs.getValue(), size.getValue(), delay.getValue(), mem_buffer.getValue(), zero_copy.getValue());
    }
  }
  catch (TCLAP::ArgException &e)  // catch any exceptions
  {
    std::cerr << "error: " << e.error() << " for arg " << e.argId() << std::endl;
    return EXIT_FAILURE;
  }

  return(0);
}
