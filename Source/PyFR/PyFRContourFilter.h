#ifndef PYFRCONTOURFILTER_H
#define PYFRCONTOURFILTER_H

#define BOOST_SP_DISABLE_THREADS

#include <string>
#include <utility>
#include <vector>

#include <vtkm/cont/cuda/DeviceAdapterCuda.h>
#include "IsosurfaceHexahedra.h"

class PyFRData;
class PyFRContourData;

class PyFRContourFilter
{
private:
  typedef vtkm::cont::DeviceAdapterTagCuda CudaTag;

  typedef vtkm::worklet::IsosurfaceFilterHexahedra<FPType,CudaTag>
  IsosurfaceFilter;

public:
  PyFRContourFilter();
  virtual ~PyFRContourFilter();

  void AddContourValue(FPType value) { this->ContourValues.push_back(value); }
  void ClearContourValues()          { this->ContourValues.clear(); }

  void SetContourField(int i) { this->ContourField = i; }

  void operator ()(PyFRData*,PyFRContourData*);
  void MapFieldOntoIsosurfaces(int,PyFRData*,PyFRContourData*);

  // Returns the min/max of the scalar we are computing the isosurface on.  Not
  // valid until operator() is applied.
  std::pair<float,float> Range() const;

protected:
  IsosurfaceFilter isosurfaceFilter;
  std::vector<FPType> ContourValues;
  int ContourField;
  float minmax[2];
};
#endif
