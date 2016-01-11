#ifndef PYFRPARALLELSLICEFILTER_H
#define PYFRPARALLELSLICEFILTER_H

#define BOOST_SP_DISABLE_THREADS

class PyFRData;
class PyFRContourData;

struct PyFRParallelSliceFilter
{
  void SetPlane(FPType,FPType,FPType,FPType,FPType,FPType) {}
  void SetSpacing(FPType) {}
  void SetNumberOfPlanes(unsigned) {}

  void operator ()(PyFRData*,PyFRContourData*) const {}
  void MapFieldOntoSlices(int,PyFRData*,PyFRContourData*) {}
};
#endif
