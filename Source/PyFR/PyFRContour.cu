#include "PyFRContour.h"

#include <vtkm/cont/DeviceAdapter.h>
#include <vtkm/cont/cuda/DeviceAdapterCuda.h>

//----------------------------------------------------------------------------
PyFRContour::ColorArrayHandle PyFRContour::GetColorData()
{
  typedef ::vtkm::cont::DeviceAdapterTagCuda CudaTag;

   vtkm::cont::ArrayHandleTransform<FPType,
    ColorArrayHandle,
    ColorTable,
    ColorTable> colorHandle(this->ColorData,this->Table,this->Table);

    vtkm::cont::DeviceAdapterAlgorithm<CudaTag>().
      Copy(this->ScalarData, colorHandle);

    return this->ColorData;
}
