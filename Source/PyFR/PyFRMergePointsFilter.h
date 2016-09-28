#ifndef PYFRMERGEPOINTSFILTER_H
#define PYFRMERGEPOINTSFILTER_H

#define BOOST_SP_DISABLE_THREADS

#include <string>
#include <vector>

#include <vtkm/cont/cuda/DeviceAdapterCuda.h>
#include "IsosurfaceHexahedra.h"

class PyFRData;

class PyFRMergePointsFilter
{
private:
  typedef vtkm::cont::DeviceAdapterTagCuda CudaTag;

public:
  PyFRMergePointsFilter();
  virtual ~PyFRMergePointsFilter();

  void operator ()(PyFRData*,PyFRData*);
};
#endif
