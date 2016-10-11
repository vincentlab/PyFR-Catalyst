#ifndef PYFRGRADIENTFILTER_H
#define PYFRGRADIENTFILTER_H

#define BOOST_SP_DISABLE_THREADS

#include <string>
#include <utility>
#include <vector>

#include "IsosurfaceHexahedra.h"
#include <vtkm/cont/cuda/DeviceAdapterCuda.h>

class PyFRData;

class PyFRGradientFilter
{
private:
  typedef vtkm::cont::DeviceAdapterTagCuda CudaTag;

public:
  PyFRGradientFilter();
  virtual ~PyFRGradientFilter();

  void operator()(PyFRData*, PyFRData*);
};
#endif
