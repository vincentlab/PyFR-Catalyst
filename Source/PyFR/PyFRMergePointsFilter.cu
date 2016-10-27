#include "PyFRMergePointsFilter.h"

#include <string>
#include <vector>

#include "PyFRData.h"
#include "MergePoints.h"
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
  const vtkm::cont::DataSet& input = inputData->GetDataSet();
  vtkm::cont::DataSet& output = outputData->GetDataSet();
  output.Clear();

  vtkm::worklet::MergePoints merge;

  vtkm::cont::Timer<CudaTag> timer;
  merge.Run(input.GetCellSet().ResetCellSetList(MergePointsFilterCellSets()),
            input, output, CudaTag());

}