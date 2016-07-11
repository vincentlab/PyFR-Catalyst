#ifndef PYFRPARALLELSLICEFILTER_H
#define PYFRPARALLELSLICEFILTER_H

#define BOOST_SP_DISABLE_THREADS

#include <string>
#include <vector>

#include <vtkm/cont/cuda/DeviceAdapterCuda.h>
#include "IsosurfaceHexahedra.h"

class PyFRData;
class PyFRContourData;

class PyFRParallelSliceFilter
{
private:
  typedef vtkm::cont::DeviceAdapterTagCuda CudaTag;

  typedef vtkm::worklet::IsosurfaceFilterHexahedra<FPType,CudaTag>
  IsosurfaceFilter;

public:
  PyFRParallelSliceFilter();
  virtual ~PyFRParallelSliceFilter();

  void SetPlane(FPType,FPType,FPType,FPType,FPType,FPType);
  void SetSpacing(FPType spacing) { this->Spacing = spacing; }
  void SetNumberOfPlanes(unsigned n) { this->NPlanes = n; }

  void operator ()(PyFRData*,PyFRContourData*);
  void MapFieldOntoSlices(int,PyFRData*,PyFRContourData*);

protected:
  IsosurfaceFilter isosurfaceFilter;
  FPType Origin[3];
  FPType Normal[3];
  FPType Spacing;
  unsigned NPlanes;
};
#endif
