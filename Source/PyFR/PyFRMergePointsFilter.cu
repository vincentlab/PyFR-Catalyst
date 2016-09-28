#include "PyFRMergePointsFilter.h"

#include <string>
#include <vector>

#include "MergePoints.h"
#include "PyFRData.h"

struct MergePointsFilterCellSets : vtkm::ListTagBase<PyFRData::CellSet>
{
};

PyFRMergePointsFilter::PyFRMergePointsFilter()
{
}

void PyFRMergePointsFilter::operator()(PyFRData* inputData,
                                       PyFRData* outputData)
{
  typedef ::vtkm::cont::DeviceAdapterTagCuda CudaTag;
  typedef PyFRData::Vec3ArrayHandle CoordinateArrayHandle;

  const vtkm::cont::DataSet& input = inputData->GetDataSet();
  vtkm::cont::DataSet& output = outputData->GetDataSet();
  output.Clear();

  CoordinateArrayHandle coords =
      input.GetCoordinateSystem().GetData().CastToArrayHandle(
          CoordinateArrayHandle::ValueType(),
          CoordinateArrayHandle::StorageTag());

  vtkm::worklet::MergePoints merge;

  merge.Run(input.GetCellSet().ResetCellSetList(MergePointsFilterCellSets()),
            coords, output, CudaTag());

  for (vtkm::IdComponent i = 0; i < input.GetNumberOfFields(); i++)
    output.AddField(input.GetField(i));
}