#include <cstddef>
#include <stdexcept>
#include <string>
#include <iostream>
#include <vector>
#include <algorithm>
#include <functional>
#include <future>
#include <random>
#include <iomanip>
#include <exception>
#include <limits>

namespace ulanova
{
  struct circle_t
  {
    double radius;
    double center_x;
    double center_y;
  };

  bool isInside(double x, double y, const circle_t& circle);
}

bool ulanova::isInside(double x, double y, const circle_t& circle)
{
  const double deltaX = x - circle.center_x;
  const double deltaY = y - circle.center_y;
  return ((deltaX * deltaX) + (deltaY * deltaY)) <= (circle.radius * circle.radius);
}

namespace ulanova
{
  struct options_t
  {
    size_t threads;
    size_t tries;
    size_t seed;
  };

  options_t parseOptions(int argc, const char* const* argv);
}

namespace
{
  const int minArgumentCount = 3;
  const int maxArgumentCount = 4;
  const int threadsIndex = 1;
  const int triesIndex = 2;
  const int seedIndex = 3;

  long long parseInteger(const std::string& text, const std::string& name)
  {
    size_t parsed_length = 0;
    long long value = 0;
    try
    {
      value = std::stoll(text, &parsed_length);
    }
    catch (const std::logic_error&)
    {
      parsed_length = 0;
    }
    if ((parsed_length == 0) || (parsed_length != text.size()))
    {
      throw std::invalid_argument(name + "must be integer");
    }
    return value;
  }
}

ulanova::options_t ulanova::parseOptions(int argc, const char* const* argv)
{
  if ((argc < minArgumentCount) || (argc > maxArgumentCount))
  {
    throw std::invalid_argument("usage: lab threads tries [seed]");
  }
  const long long threads = parseInteger(argv[threadsIndex], "threads");
  const long long tries = parseInteger(argv[triesIndex], "tries");
  const long long seed = (argc > seedIndex) ? parseInteger(argv[seedIndex], "seed") : 0;

  if (threads < 0)
  {
    throw std::invalid_argument("threads must not be negative");
  }
  if (tries <= 0)
  {
    throw std::invalid_argument("tries must be positive");
  }
  if (seed < 0)
  {
    throw std::invalid_argument("seed must not be negative");
  }
  options_t options = {0, 0, 0};
  options.threads = static_cast< size_t >(threads);
  options.tries = static_cast< size_t >(tries);
  options.seed = static_cast< size_t >(seed);
  return options;
}

namespace ulanova
{
  std::vector< circle_t > readCircles(std::istream& input);
}

namespace
{
  const char* const invalidCircleMessage = "invalid figure parameters";
}

std::vector< ulanova::circle_t > ulanova::readCircles(std::istream& input)
{
  std::vector< circle_t > circles;
  long long radius = 0;
  while (input >> radius)
  {
    long long ignored = 0;
    long long center_x = 0;
    long long center_y = 0;
    if (!(input >> ignored >> center_x >> center_y))
    {
      throw std::invalid_argument(invalidCircleMessage);
    }
    circles.push_back(
        {static_cast< double >(radius), static_cast< double >(center_x), static_cast< double >(center_y)});
  }
  if (!input.eof())
  {
    throw std::invalid_argument(invalidCircleMessage);
  }
  return circles;
}

namespace ulanova
{
  struct areas_t
  {
    double covered;
    double intersection;
  };
  areas_t estimateAreas(const std::vector< circle_t >& circles, size_t threads, size_t tries, size_t seed);
}

namespace
{
  struct box_t
  {
    double min_x;
    double max_x;
    double min_y;
    double max_y;
  };

  struct hits_t
  {
    size_t covered;
    size_t intersection;
  };

  const size_t minThreads = 1;
  const size_t maxThreads = 1000;

  box_t findBoundingBox(const std::vector< ulanova::circle_t >& circles)
  {
    const ulanova::circle_t& first = circles.front();
    box_t box = {first.center_x - first.radius, first.center_x + first.radius, first.center_y - first.radius,
        first.center_y + first.radius};
    for (const ulanova::circle_t& circle : circles)
    {
      box.min_x = std::min(box.min_x, circle.center_x - circle.radius);
      box.max_x = std::max(box.max_x, circle.center_x + circle.radius);
      box.min_y = std::min(box.min_y, circle.center_y - circle.radius);
      box.max_y = std::max(box.max_y, circle.center_y + circle.radius);
    }
    return box;
  }

  size_t countContaining(double x, double y, const std::vector< ulanova::circle_t >& circles)
  {
    const auto count = std::count_if(circles.begin(), circles.end(),
        [x, y](const ulanova::circle_t& circle) -> bool
        {
          return ulanova::isInside(x, y, circle);
        });
    return static_cast< size_t >(count);
  }

  hits_t countHits(const box_t& box, const std::vector< ulanova::circle_t >& circles, size_t tries, size_t seed)
  {
    std::default_random_engine engine(seed);

    std::uniform_real_distribution< double > distribution_x(box.min_x, box.max_x);
    std::uniform_real_distribution< double > distribution_y(box.min_y, box.max_y);

    hits_t hits = {0, 0};

    for (size_t i = 0; i < tries; ++i)
    {
      const double x = distribution_x(engine);
      const double y = distribution_y(engine);
      const size_t contaning = countContaining(x, y, circles);
      if (contaning > 0)
      {
        ++hits.covered;
      }
      if (contaning == circles.size())
      {
        ++hits.intersection;
      }
    }
    return hits;
  }
}

ulanova::areas_t ulanova::estimateAreas(
    const std::vector< circle_t >& circles, size_t threads, size_t tries, size_t seed)
{
  if (circles.empty())
  {
    return {0.0, 0.0};
  }
  const box_t box = findBoundingBox(circles);
  const size_t workers = std::min(std::max(threads, minThreads), maxThreads);
  const size_t baseTries = tries / workers;
  const size_t extraTries = tries % workers;

  std::vector< std::future< hits_t > > futures;
  futures.reserve(workers);
  for (size_t i = 0; i < workers; ++i)
  {
    size_t work_tries = baseTries;
    if (i < extraTries)
    {
      ++work_tries;
    }
    futures.push_back(std::async(std::launch::async, countHits, box, std::cref(circles), work_tries, seed + i));
  }

  hits_t total = {0, 0};
  for (std::future< hits_t >& result : futures)
  {
    const hits_t hits = result.get();
    total.covered += hits.covered;
    total.intersection += hits.intersection;
  }

  const double scale = ((box.max_x - box.min_x) * (box.max_y - box.min_y)) / static_cast< double >(tries);
  const double covered = scale * static_cast< double >(total.covered);
  const double intersection = scale * static_cast< double >(total.intersection);
  return {covered, intersection};
}

int main(int argc, char* argv[])
{
  try
  {
    const ulanova::options_t options = ulanova::parseOptions(argc, argv);
    const std::vector< ulanova::circle_t > circles = ulanova::readCircles(std::cin);
    const ulanova::areas_t areas = ulanova::estimateAreas(circles, options.threads, options.tries, options.seed);
    std::cout << std::setprecision(std::numeric_limits< double >::max_digits10);
    std::cout << areas.covered << ' ' << areas.intersection << '\n';
    if (!std::cout)
    {
      throw std::runtime_error("failed to write result");
    }
  }
  catch (const std::invalid_argument& error)
  {
    std::cerr << "error: " << error.what() << '\n';
    return 1;
  }
  catch (const std::exception& error)
  {
    std::cerr << "internal error: " << error.what() << '\n';
    return 2;
  }
  return 0;
}
