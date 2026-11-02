#ifndef CLI_HPP
#define CLI_HPP

#include <string>
#include <vector>
#include <cstddef>
#include "area_calc.hpp"
#include "interprocess_connect.hpp"

namespace ulanova
{
  struct NamedSet
  {
    std::string name;
    std::vector< size_t > circle_indices;
  };

  struct CalcActive
  {
    std::string calc_name;
    std::string process_name;
    std::string set_name;
    size_t threads;
    size_t tries;
    bool is_done;
    double area_result;
    long long duration_ms;
  };

  struct CliContext
  {
    std::vector< circle_t > storage;
    std::vector< std::string > names;
    std::vector< NamedSet > sets;
    std::vector< std::string > active_processes;
    std::vector< CalcActive > calculations;
  };

  class CLI
  {
  public:
    CLI() = default;

    void run();

  private:
    CliContext ctx_;

    void processLine(const std::string& line);

    void skipWs(const std::string& str, std::string::size_type& pos);
    std::string readName(const std::string& str, std::string::size_type& pos);
    bool readInt(const std::string& str, std::string::size_type& pos, int& val);
    bool readSize(const std::string& str, std::string::size_type& pos, size_t& val);
    bool checkEnd(const std::string& str, std::string::size_type pos);

    void parseCircle(const std::string& line, std::string::size_type& pos);
    void parseSet(const std::string& line, std::string::size_type& pos);
    void parseShow(const std::string& line, std::string::size_type& pos);
    void parseShowSet(const std::string& line, std::string::size_type& pos);
    void parseFrame(const std::string& line, std::string::size_type& pos);
    void parseFrameset(const std::string& line, std::string::size_type& pos);
    void parseSpawn(const std::string& line, std::string::size_type& pos);
    void parseAreaOn(const std::string& line, std::string::size_type& pos);
    void parseStatus(const std::string& line, std::string::size_type& pos);
    void parseWait(const std::string& line, std::string::size_type& pos);

    ptrdiff_t getCircleIdx(const std::string& name) const;
    ptrdiff_t getSetIdx(const std::string& name) const;
    ptrdiff_t getProcessIdx(const std::string& name) const;
    ptrdiff_t getCalcIdx(const std::string& name) const;

    void executeSystemSpawn(const std::string& process_name, unsigned int seed);
    void executeSystemAreaOn(CalcActive& calc);
    void executeSystemWait(CalcActive& calc);
  };
}

#endif
