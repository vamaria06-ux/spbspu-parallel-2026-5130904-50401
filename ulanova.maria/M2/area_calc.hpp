#ifndef AREA_CALC_HPP
#define AREA_CALC_HPP

#include <cstddef>
#include <vector>

namespace ulanova
{
  struct circle_t
  {
    int r;
    int x, y;
  };

  struct set_t
  {
    std::vector< const circle_t* > shapes;
  };

  struct rect_t
  {
    double xmin, xmax, ymin, ymax;
  };

  rect_t bounds(const circle_t* const* b, const circle_t* const* e);
  double area(const circle_t* const* b, const circle_t* const* e, size_t thrds, size_t tests, size_t seed);
  size_t calc(const circle_t* const* b, const circle_t* const* e, rect_t rect, size_t tests, size_t seed);
  bool isInside(double x, double y, const circle_t& cl);
}

#endif
