#include <exception>
#include <iostream>
#include <signal.h>
#include "cli.hpp"

int main()
{
  struct sigaction sa{};
  sa.sa_handler = SIG_IGN;
  sigaction(SIGPIPE, &sa, nullptr);

  try
  {
    ulanova::CLI app;
    app.run();
  }
  catch (const std::exception& e)
  {
    std::cerr << "Fatal error: " << e.what() << "\n";
    return 1;
  }
  catch (...)
  {
    std::cerr << "Unknown fatal error occurred" << "\n";
    return 1;
  }

  return 0;
}
