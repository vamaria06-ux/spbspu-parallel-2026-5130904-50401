#include "area_calc.hpp"
#include <algorithm>
#include <future>
#include <random>
#include <vector>

namespace ulanova
{
  bool isInside(double x, double y, const circle_t& cl)
  {
    const double dx = x - cl.x;
    const double dy = y - cl.y;
    return (dx * dx + dy * dy) <= (static_cast< double >(cl.r) * cl.r);
  }

  rect_t bounds(const circle_t* const* b, const circle_t* const* e)
  {
    if (b == e)
    {
      return rect_t{0.0, 0.0, 0.0, 0.0};
    }

    double xmin = static_cast< double >((*b)->x - (*b)->r);
    double xmax = static_cast< double >((*b)->x + (*b)->r);
    double ymin = static_cast< double >((*b)->y - (*b)->r);
    double ymax = static_cast< double >((*b)->y + (*b)->r);

    for (const circle_t* const* it = b + 1; it != e; ++it)
    {
      xmin = std::min(xmin, static_cast< double >((*it)->x - (*it)->r));
      xmax = std::max(xmax, static_cast< double >((*it)->x + (*it)->r));
      ymin = std::min(ymin, static_cast< double >((*it)->y - (*it)->r));
      ymax = std::max(ymax, static_cast< double >((*it)->y + (*it)->r));
    }

    return rect_t{xmin, xmax, ymin, ymax};
  }

  size_t calc(const circle_t* const* b, const circle_t* const* e, rect_t rect, size_t tests, size_t seed)
  {
    std::default_random_engine eng(static_cast< unsigned int >(seed));
    const double range = static_cast< double >(eng.max() - eng.min());
    const double kx = (rect.xmax - rect.xmin) / range;
    const double ky = (rect.ymax - rect.ymin) / range;

    size_t res{};

    for (size_t i = 0; i < tests; ++i)
    {
      const double x = rect.xmin + static_cast< double >(eng() - eng.min()) * kx;
      const double y = rect.ymin + static_cast< double >(eng() - eng.min()) * ky;
      bool inside = false;

      for (const circle_t* const* it = b; it != e && !inside; ++it)
      {
        inside = isInside(x, y, **it);
      }
      res += inside;
    }
    return res;
  }

  double area(const circle_t* const* b, const circle_t* const* e, size_t thrds, size_t tests, size_t seed)
  {
    if (b == e || tests == 0 || thrds == 0)
    {
      return 0.0;
    }

    const rect_t rect = bounds(b, e);
    const double box_area = (rect.xmax - rect.xmin) * (rect.ymax - rect.ymin);

    const size_t tests_per_thread = tests / thrds;
    const size_t remainder = tests % thrds;

    std::vector< std::future< size_t > > futures;
    futures.reserve(thrds);

    for (size_t i = 0; i < thrds; ++i)
    {
      const size_t current_tests = tests_per_thread + (i == 0 ? remainder : 0);
      const size_t current_seed = seed + i;

      futures.push_back(std::async(std::launch::async, calc, b, e, rect, current_tests, current_seed));
    }

    size_t total_hits = 0;
    for (auto& f : futures)
    {
      total_hits += f.get();
    }

    return (static_cast< double >(total_hits) / static_cast< double >(tests)) * box_area;
  }
}
