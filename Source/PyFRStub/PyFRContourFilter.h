#ifndef PYFRCONTOURFILTER_H
#define PYFRCONTOURFILTER_H

#include <limits>
#include <utility>
class PyFRData;
class PyFRContourData;

struct PyFRContourFilter
{
  void operator ()(PyFRData*,PyFRContourData*) const {}
  void MapFieldOntoIsosurfaces(int,PyFRData*,PyFRContourData*) {}

  void AddContourValue(FPType) {}
  void ClearContourValues() {}

  void SetContourField(int) {}
  std::pair<float,float> Range() {
    return std::make_pair(
      std::numeric_limits<float>::signaling_NaN(),
      std::numeric_limits<float>::signaling_NaN()
    );
  }
};
#endif
