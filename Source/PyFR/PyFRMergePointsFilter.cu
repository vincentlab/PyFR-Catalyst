#include "PyFRMergePointsFilter.h"

#include <string>
#include <vector>

#include "MergePoints.h"
#include "PyFRData.h"
#include <vtkm/cont/Timer.h>

struct MergePointsFilterCellSets : vtkm::ListTagBase<PyFRData::CellSet>
{
};

PyFRMergePointsFilter::PyFRMergePointsFilter()
{
}

PyFRMergePointsFilter::~PyFRMergePointsFilter()
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

  vtkm::cont::Timer<CudaTag> timer;
  merge.Run(input.GetCellSet().ResetCellSetList(MergePointsFilterCellSets()),
            coords, output, CudaTag());

  std::cout << "time to compute merge points: " << timer.GetElapsedTime() << std::endl;

  for (vtkm::IdComponent i = 0; i < input.GetNumberOfFields(); i++)
    output.AddField(input.GetField(i));
}