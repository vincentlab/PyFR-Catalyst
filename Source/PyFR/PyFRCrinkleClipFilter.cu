#include "PyFRCrinkleClipFilter.h"

#include <string>
#include <vector>

#include <vtkm/BinaryPredicates.h>
#include <vtkm/ImplicitFunctions.h>
#include <vtkm/cont/cuda/DeviceAdapterCuda.h>

#include "CrinkleClip.h"
#include "PyFRData.h"

PyFRCrinkleClipFilter::PyFRCrinkleClipFilter()
{
  this->Origin[0] = this->Origin[1] = this->Origin[2] = 0.;
  this->Normal[0] = this->Normal[1] = 0.;
  this->Normal[2] = 1.;
}

void PyFRCrinkleClipFilter::SetPlane(FPType origin_x,
                                     FPType origin_y,
                                     FPType origin_z,
                                     FPType normal_x,
                                     FPType normal_y,
                                     FPType normal_z)
{
  this->Origin[0] = origin_x;
  this->Origin[1] = origin_y;
  this->Origin[2] = origin_z;
  this->Normal[0] = normal_x;
  this->Normal[1] = normal_y;
  this->Normal[2] = normal_z;
}

void PyFRCrinkleClipFilter::operator ()(PyFRData* inputData,
                                        PyFRData* outputData) const
{
  typedef ::vtkm::cont::DeviceAdapterTagCuda CudaTag;
  typedef vtkm::worklet::CrinkleClip<CudaTag> CrinkleClip;
  typedef PyFRData::Vec3ArrayHandle CoordinateArrayHandle;
  typedef vtkm::ListTagBase<PyFRData::CellSet> CellSetTag;
  typedef vtkm::Plane ImplicitFunction;

  ImplicitFunction func(vtkm::Vec<FPType,3>(this->Origin[0],
                                            this->Origin[1],
                                            this->Origin[2]),
                        vtkm::Vec<FPType,3>(this->Normal[0],
                                            this->Normal[1],
                                            this->Normal[2]));

  const vtkm::cont::DataSet& input = inputData->GetDataSet();
  vtkm::cont::DataSet& output = outputData->GetDataSet();
  output.Clear();

  CoordinateArrayHandle coords = input.GetCoordinateSystem().GetData()
    .CastToArrayHandle(CoordinateArrayHandle::ValueType(),
                       CoordinateArrayHandle::StorageTag());

  vtkm::ImplicitFunctionValue<ImplicitFunction> function(func);

  vtkm::cont::ArrayHandleTransform<FPType,CoordinateArrayHandle,
    vtkm::ImplicitFunctionValue<ImplicitFunction> > dataArray(coords,function);

  vtkm::cont::ArrayHandleConstant<FPType> clipArray(0.,coords.GetNumberOfValues());

  CrinkleClip crinkleClip;

  crinkleClip.Run(dataArray,
                  clipArray,
                  vtkm::SortLess(),
                  input.GetCellSet().ResetCellSetList(CellSetTag()),
                  input.GetCoordinateSystem(),
                  output);

  for (vtkm::IdComponent i=0;i<input.GetNumberOfFields();i++)
    output.AddField(input.GetField(i));
}
