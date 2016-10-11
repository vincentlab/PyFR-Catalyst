#ifndef PYFRGRADIENTFILTER_H
#define PYFRGRADIENTFILTER_H

class PyFRData;

struct PyFRGradientFilter
{
  void operator ()(PyFRData*,PyFRData*) const {}
};
#endif
