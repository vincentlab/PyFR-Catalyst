
#include "PyFRGradientFilter.h"

#include "Gradients.h"
#include "PyFRData.h"

#include <vtkm/cont/Timer.h>

struct GradientFilterCellSets : vtkm::ListTagBase<PyFRData::CellSet>
{
};

//----------------------------------------------------------------------------
PyFRGradientFilter::PyFRGradientFilter()
{
}

//----------------------------------------------------------------------------
PyFRGradientFilter::~PyFRGradientFilter()
{
}

//----------------------------------------------------------------------------
void
PyFRGradientFilter::operator()(PyFRData* inputData, PyFRData* outputData)
{
  const vtkm::cont::DataSet& input = inputData->GetDataSet();
  vtkm::cont::DataSet& output = outputData->GetDataSet();
  output.Clear();

  vtkm::cont::Timer<CudaTag> timer;

  output.AddCoordinateSystem(input.GetCoordinateSystem());
  output.AddCellSet(input.GetCellSet());

  vtkm::worklet::Gradients gradients;

  vtkm::cont::ArrayHandle<FPType> densityGradientMag;
  vtkm::cont::ArrayHandle<FPType> pressureGradientMag;
  vtkm::cont::ArrayHandle<FPType> velocityGradientMag;
  vtkm::cont::ArrayHandle<FPType> velocityQCriterion;

  gradients.Run(input.GetCellSet().ResetCellSetList(GradientFilterCellSets()),
    input, densityGradientMag, pressureGradientMag, velocityGradientMag,
    velocityQCriterion, output, CudaTag());

  for (vtkm::IdComponent i = 0; i < input.GetNumberOfFields(); i++)
  {
    output.AddField(input.GetField(i));
  }

  // Now add the new gradient fields to the output data
  // in the following order
  //  fieldName[5] = "density_gradient_magnitude";
  //  fieldName[6] = "pressure_gradient_magnitude";
  //  fieldName[7] = "velocity_gradient_magnitude";
  //  fieldName[8] = "velocity_qcriterion";
  // These need to go to the end so that the original fields order
  // is kept the same

  enum ElemType
  {
    CONSTANT = 0,
    LINEAR = 1,
    QUADRATIC = 2
  };
  vtkm::cont::Field dgrad("density_gradient_magnitude", LINEAR,
    vtkm::cont::Field::ASSOC_POINTS, densityGradientMag);
  vtkm::cont::Field pgrad("pressure_gradient_magnitude", LINEAR,
    vtkm::cont::Field::ASSOC_POINTS, pressureGradientMag);
  vtkm::cont::Field vgrad("velocity_gradient_magnitude", LINEAR,
    vtkm::cont::Field::ASSOC_POINTS, velocityGradientMag);
  vtkm::cont::Field qcriterion("velocity_qcriterion", LINEAR,
    vtkm::cont::Field::ASSOC_POINTS, velocityQCriterion);

  output.AddField(dgrad);
  output.AddField(pgrad);
  output.AddField(vgrad);
  output.AddField(qcriterion);

  std::cout << "time to compute gradients " << timer.GetElapsedTime()
            << std::endl;
}
