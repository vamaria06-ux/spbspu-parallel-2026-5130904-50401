#include "compute.hpp"
#include <vector>
#include "interprocess_connect.hpp"
#include "area_calc.hpp"

namespace ulanova
{
  void computeProcess(int read_fd, int write_fd, const char*)
  {
    bool running = true;

    while (running)
    {
      request_t req{};
      try
      {
        recvAll(read_fd, &req, sizeof(request_t));
      }
      catch (...)
      {
        break;
      }

      if (req.command == cmd_t::EXIT)
      {
        running = false;
        continue;
      }

      if (req.command == cmd_t::CALC)
      {
        response_t resp{};
        try
        {
          std::vector< circle_t > shapes_data(req.shapes_count);
          recvAll(read_fd, shapes_data.data(), shapes_data.size() * sizeof(circle_t));
          std::vector< const circle_t* > shapes_ptrs(req.shapes_count);
          for (size_t i = 0; i < req.shapes_count; ++i)
          {
            shapes_ptrs[i] = &shapes_data[i];
          }

          const auto shapes_end = shapes_ptrs.data() + req.shapes_count;
          const size_t hits = calc(shapes_ptrs.data(), shapes_end, req.rect, req.tests, req.seed);

          resp.status = status_t::SUCCESS;
          resp.hits = hits;
        }
        catch (...)
        {
          resp.status = status_t::ERROR;
          resp.hits = 0;
        }

        try
        {
          sendAll(write_fd, &resp, sizeof(response_t));
        }
        catch (...)
        {
          break;
        }
      }
    }
  }
}
