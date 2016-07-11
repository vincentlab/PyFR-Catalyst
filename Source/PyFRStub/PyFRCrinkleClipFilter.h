#ifndef PYFRCRINKLECLIPFILTER_H
#define PYFRCRINKLECLIPFILTER_H

class PyFRData;

struct PyFRCrinkleClipFilter
{
  void SetPlane(FPType,FPType,FPType,FPType,FPType,FPType) {}

  void operator ()(PyFRData*,PyFRData*) const {}
};
#endif
