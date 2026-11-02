#include "cli.hpp"
#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <iomanip>
#include <unistd.h>
#include "compute.hpp"

namespace ulanova
{
  static int parent_read_fd = -1;
  static int parent_write_fd = -1;
  static const char* shared_memory_name = "/ulanova_shm";

  void CLI::skipWs(const std::string& str, std::string::size_type& pos)
  {
    while (pos < str.size() && std::isspace(static_cast< unsigned char >(str[pos])))
    {
      ++pos;
    }
  }

  std::string CLI::readName(const std::string& str, std::string::size_type& pos)
  {
    skipWs(str, pos);
    if (pos >= str.size() || !std::isalpha(static_cast< unsigned char >(str[pos])))
    {
      return "";
    }
    const std::string::size_type start = pos;
    while (pos < str.size() && !std::isspace(static_cast< unsigned char >(str[pos])))
    {
      ++pos;
    }
    return str.substr(start, pos - start);
  }

  bool CLI::readInt(const std::string& str, std::string::size_type& pos, int& val)
  {
    skipWs(str, pos);
    if (pos >= str.size())
    {
      return false;
    }
    std::string::size_type next = 0;
    try
    {
      val = std::stoi(str.substr(pos), &next);
      pos += next;
      return true;
    }
    catch (...)
    {
      return false;
    }
  }

  bool CLI::readSize(const std::string& str, std::string::size_type& pos, size_t& val)
  {
    skipWs(str, pos);
    if (pos >= str.size() || str[pos] == '-')
    {
      return false;
    }
    std::string::size_type next = 0;
    try
    {
      val = std::stoull(str.substr(pos), &next);
      pos += next;
      return true;
    }
    catch (...)
    {
      return false;
    }
  }

  bool CLI::checkEnd(const std::string& str, std::string::size_type pos)
  {
    skipWs(str, pos);
    return pos >= str.size();
  }

  ptrdiff_t CLI::getCircleIdx(const std::string& name) const
  {
    const auto it = std::find(ctx_.names.cbegin(), ctx_.names.cend(), name);
    if (it == ctx_.names.cend())
    {
      return -1;
    }
    return std::distance(ctx_.names.cbegin(), it);
  }

  ptrdiff_t CLI::getSetIdx(const std::string& name) const
  {
    for (size_t i = 0; i < ctx_.sets.size(); ++i)
    {
      if (ctx_.sets[i].name == name)
      {
        return static_cast< ptrdiff_t >(i);
      }
    }
    return -1;
  }

  ptrdiff_t CLI::getProcessIdx(const std::string& name) const
  {
    const auto it = std::find(ctx_.active_processes.cbegin(), ctx_.active_processes.cend(), name);
    if (it == ctx_.active_processes.cend())
    {
      return -1;
    }
    return std::distance(ctx_.active_processes.cbegin(), it);
  }

  ptrdiff_t CLI::getCalcIdx(const std::string& name) const
  {
    for (size_t i = 0; i < ctx_.calculations.size(); ++i)
    {
      if (ctx_.calculations[i].calc_name == name)
      {
        return static_cast< ptrdiff_t >(i);
      }
    }
    return -1;
  }

  void CLI::run()
  {
    std::string line;
    while (std::getline(std::cin, line))
    {
      if (!line.empty())
      {
        processLine(line);
      }
    }
  }

  void CLI::processLine(const std::string& line)
  {
    std::string::size_type pos = 0;
    const std::string cmd = readName(line, pos);

    if (cmd.empty())
    {
      return;
    }

    if (cmd == "circle")
    {
      parseCircle(line, pos);
    }
    else if (cmd == "set")
    {
      parseSet(line, pos);
    }
    else if (cmd == "show")
    {
      parseShow(line, pos);
    }
    else if (cmd == "showset")
    {
      parseShowSet(line, pos);
    }
    else if (cmd == "frame")
    {
      parseFrame(line, pos);
    }
    else if (cmd == "frameset")
    {
      parseFrameset(line, pos);
    }
    else if (cmd == "spawn")
    {
      parseSpawn(line, pos);
    }
    else if (cmd == "area-on")
    {
      parseAreaOn(line, pos);
    }
    else if (cmd == "status")
    {
      parseStatus(line, pos);
    }
    else if (cmd == "wait")
    {
      parseWait(line, pos);
    }
    else
    {
      std::cout << "<UNKNOWN COMMAND>\n";
    }
  }

  void CLI::parseCircle(const std::string& line, std::string::size_type& pos)
  {
    const std::string name = readName(line, pos);
    int r = 0;
    int x = 0;
    int y = 0;

    if (name.empty() || !readInt(line, pos, r) || !readInt(line, pos, x) || !readInt(line, pos, y)
        || !checkEnd(line, pos) || getCircleIdx(name) != -1 || getSetIdx(name) != -1 || r <= 0)
    {
      std::cout << "<INVALID COMMAND>\n";
      return;
    }

    ctx_.storage.push_back(circle_t{r, x, y});
    ctx_.names.push_back(name);
  }

  void CLI::parseSet(const std::string& line, std::string::size_type& pos)
  {
    const std::string name = readName(line, pos);
    size_t expected_size = 0;

    if (name.empty() || !readSize(line, pos, expected_size) || getCircleIdx(name) != -1 || getSetIdx(name) != -1)
    {
      std::cout << "<INVALID COMMAND>\n";
      return;
    }

    NamedSet new_set{name, {}};
    for (size_t i = 0; i < expected_size; ++i)
    {
      const std::string shape_name = readName(line, pos);
      const ptrdiff_t idx = getCircleIdx(shape_name);
      if (shape_name.empty() || idx == -1)
      {
        std::cout << "<INVALID COMMAND>\n";
        return;
      }
      new_set.circle_indices.push_back(static_cast< size_t >(idx));
    }

    if (!checkEnd(line, pos))
    {
      std::cout << "<INVALID COMMAND>\n";
      return;
    }
    ctx_.sets.push_back(new_set);
  }

  void CLI::parseShow(const std::string& line, std::string::size_type& pos)
  {
    const std::string name = readName(line, pos);
    const ptrdiff_t idx = getCircleIdx(name);

    if (name.empty() || !checkEnd(line, pos) || idx == -1)
    {
      std::cout << "<INVALID COMMAND>\n";
      return;
    }

    const circle_t& c = ctx_.storage[idx];
    std::cout << c.r << " (" << c.x << " " << c.y << ")\n";
  }

  void CLI::parseShowSet(const std::string& line, std::string::size_type& pos)
  {
    const std::string name = readName(line, pos);
    const ptrdiff_t idx = getSetIdx(name);

    if (name.empty() || !checkEnd(line, pos) || idx == -1)
    {
      std::cout << "<INVALID COMMAND>\n";
      return;
    }

    const NamedSet& s = ctx_.sets[idx];
    for (const size_t c_idx : s.circle_indices)
    {
      const circle_t& c = ctx_.storage[c_idx];
      std::cout << c.r << " (" << c.x << " " << c.y << ")\n";
    }
  }

  void CLI::parseFrame(const std::string& line, std::string::size_type& pos)
  {
    const std::string name = readName(line, pos);
    const ptrdiff_t idx = getCircleIdx(name);

    if (name.empty() || !checkEnd(line, pos) || idx == -1)
    {
      std::cout << "<INVALID COMMAND>\n";
      return;
    }

    const circle_t& c = ctx_.storage[idx];
    const circle_t* arr[] = {&c};
    const rect_t r = bounds(arr, arr + 1);
    std::cout << "(" << r.xmin << " " << r.ymin << ") (" << r.xmax << " " << r.ymax << ")\n";
  }

  void CLI::parseFrameset(const std::string& line, std::string::size_type& pos)
  {
    const std::string name = readName(line, pos);
    const ptrdiff_t idx = getSetIdx(name);

    if (name.empty() || !checkEnd(line, pos) || idx == -1)
    {
      std::cout << "<INVALID COMMAND>\n";
      return;
    }

    const NamedSet& s = ctx_.sets[idx];
    std::vector< const circle_t* > ptrs;
    for (const size_t c_idx : s.circle_indices)
    {
      ptrs.push_back(&ctx_.storage[c_idx]);
    }

    const rect_t r = bounds(ptrs.data(), ptrs.data() + ptrs.size());
    std::cout << "(" << r.xmin << " " << r.ymin << ") (" << r.xmax << " " << r.ymax << ")\n";
  }

  void CLI::parseSpawn(const std::string& line, std::string::size_type& pos)
  {
    const std::string p_name = readName(line, pos);
    size_t seed_val = 0;

    if (p_name.empty() || !readSize(line, pos, seed_val) || !checkEnd(line, pos) || getProcessIdx(p_name) != -1)
    {
      std::cout << "<INVALID COMMAND>\n";
      return;
    }

    ctx_.active_processes.push_back(p_name);
    executeSystemSpawn(p_name, static_cast< unsigned int >(seed_val));
  }

  void CLI::parseAreaOn(const std::string& line, std::string::size_type& pos)
  {
    const std::string p_name = readName(line, pos);
    const std::string c_name = readName(line, pos);
    const std::string set_name = readName(line, pos);
    size_t thrs = 0;
    size_t tries = 0;

    if (p_name.empty() || c_name.empty() || set_name.empty())
    {
      std::cout << "<INVALID COMMAND>\n";
      return;
    }
    if (!readSize(line, pos, thrs) || !readSize(line, pos, tries) || !checkEnd(line, pos))
    {
      std::cout << "<INVALID COMMAND>\n";
      return;
    }
    if (getProcessIdx(p_name) == -1 || getSetIdx(set_name) == -1 || getCalcIdx(c_name) != -1)
    {
      std::cout << "<INVALID COMMAND>\n";
      return;
    }
    if (thrs == 0 || tries == 0)
    {
      std::cout << "<INVALID COMMAND>\n";
      return;
    }

    const CalcActive new_calc{c_name, p_name, set_name, thrs, tries, false, 0.0, 0};
    ctx_.calculations.push_back(new_calc);
    executeSystemAreaOn(ctx_.calculations.back());
  }

  void CLI::parseStatus(const std::string& line, std::string::size_type& pos)
  {
    const std::string c_name = readName(line, pos);
    const ptrdiff_t idx = getCalcIdx(c_name);

    if (c_name.empty() || !checkEnd(line, pos) || idx == -1)
    {
      std::cout << "<INVALID COMMAND>\n";
      return;
    }

    const CalcActive& c = ctx_.calculations[idx];
    if (c.is_done)
    {
      std::cout << c.process_name << " " << c.duration_ms << " ";
      std::cout << std::fixed << std::setprecision(3) << c.area_result << "\n";
    }
    else
    {
      std::cout << c.process_name << " <IN PROGRESS>\n";
    }
  }

  void CLI::parseWait(const std::string& line, std::string::size_type& pos)
  {
    const std::string c_name = readName(line, pos);
    const ptrdiff_t idx = getCalcIdx(c_name);

    if (c_name.empty() || !checkEnd(line, pos) || idx == -1)
    {
      std::cout << "<INVALID COMMAND>\n";
      return;
    }

    CalcActive& c = ctx_.calculations[idx];
    if (!c.is_done)
    {
      executeSystemWait(c);
    }
    std::cout << c.process_name << " " << c.duration_ms << " ";
    std::cout << std::fixed << std::setprecision(3) << c.area_result << "\n";
  }

  void CLI::executeSystemSpawn(const std::string&, unsigned int)
  {
    int pipe_to_child[2]{};
    int pipe_to_parent[2]{};

    if (pipe(pipe_to_child) == -1 || pipe(pipe_to_parent) == -1)
    {
      std::cout << "<INVALID COMMAND>\n";
      return;
    }

    const pid_t pid = fork();
    if (pid == -1)
    {
      std::cout << "<INVALID COMMAND>\n";
      ::close(pipe_to_child[0]);
      ::close(pipe_to_child[1]);
      ::close(pipe_to_parent[0]);
      ::close(pipe_to_parent[1]);
      return;
    }

    if (pid == 0)
    {
      ::close(pipe_to_child[1]);
      ::close(pipe_to_parent[0]);

      computeProcess(pipe_to_child[0], pipe_to_parent[1], shared_memory_name);

      ::close(pipe_to_child[0]);
      ::close(pipe_to_parent[1]);
      std::exit(0);
    }
    else
    {
      ::close(pipe_to_child[0]);
      ::close(pipe_to_parent[1]);

      parent_write_fd = pipe_to_child[1];
      parent_read_fd = pipe_to_parent[0];
    }
  }
  void CLI::executeSystemAreaOn(CalcActive& calc)
  {
    if (parent_write_fd == -1 || parent_read_fd == -1)
    {
      std::cout << "<INVALID COMMAND>\n";
      return;
    }

    const ptrdiff_t set_idx = getSetIdx(calc.set_name);
    if (set_idx == -1)
    {
      return;
    }

    const NamedSet& s = ctx_.sets[set_idx];
    const size_t shapes_count = s.circle_indices.size();

    request_t req{};
    req.command = cmd_t::CALC;
    req.shapes_count = shapes_count;
    req.tests = calc.tries;
    constexpr unsigned int default_seed = 42;
    req.seed = default_seed;
    req.thrds = calc.threads;

    std::vector< const circle_t* > ptrs;
    for (const size_t c_idx : s.circle_indices)
    {
      ptrs.push_back(&ctx_.storage[c_idx]);
    }
    req.rect = bounds(ptrs.data(), ptrs.data() + ptrs.size());

    std::vector< circle_t > shapes;
    shapes.reserve(shapes_count);
    for (const size_t c_idx : s.circle_indices)
    {
      shapes.push_back(ctx_.storage[c_idx]);
    }

    try
    {
      sendAll(parent_write_fd, &req, sizeof(request_t));
      sendAll(parent_write_fd, shapes.data(), shapes.size() * sizeof(circle_t));
    }
    catch (...)
    {
      std::cout << "<INVALID COMMAND>\n";
    }
  }

  void CLI::executeSystemWait(CalcActive& calc)
  {
    if (parent_read_fd == -1)
    {
      return;
    }

    response_t resp{};
    const auto start_time = std::chrono::steady_clock::now();

    try
    {
      recvAll(parent_read_fd, &resp, sizeof(response_t));
      const auto end_time = std::chrono::steady_clock::now();

      if (resp.status == status_t::SUCCESS)
      {
        calc.is_done = true;
        calc.duration_ms = std::chrono::duration_cast< std::chrono::milliseconds >(end_time - start_time).count();
        const ptrdiff_t set_idx = getSetIdx(calc.set_name);
        std::vector< const circle_t* > ptrs;
        for (const size_t c_idx : ctx_.sets[set_idx].circle_indices)
        {
          ptrs.push_back(&ctx_.storage[c_idx]);
        }
        const rect_t r = bounds(ptrs.data(), ptrs.data() + ptrs.size());
        const double box_area = (r.xmax - r.xmin) * (r.ymax - r.ymin);

        calc.area_result = (static_cast< double >(resp.hits) / calc.tries) * box_area;
      }
    }
    catch (...)
    {
      std::cout << "<INVALID COMMAND>\n";
    }
  }
}
