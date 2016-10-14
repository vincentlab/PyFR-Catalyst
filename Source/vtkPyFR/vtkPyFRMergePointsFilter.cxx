#include "vtkPyFRMergePointsFilter.h"

#include <vtkDataObject.h>
#include <vtkDataObjectTypes.h>
#include "vtkImplicitFunction.h"
#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkInstantiator.h>
#include <vtkObjectFactory.h>
#include <vtkPlane.h>
#include <vtkNew.h>
#include "PyFRMergePointsFilter.h"

#include "vtkPyFRData.h"

//----------------------------------------------------------------------------
int vtkPyFRMergePointsFilter::RegisterPyFRDataType()
{
  vtkInstantiator::RegisterInstantiator("vtkPyFRData",
                                        &New_vtkPyFRData);
  return 1;
}

int vtkPyFRMergePointsFilter::PyFRDataTypeRegistered =
  vtkPyFRMergePointsFilter::RegisterPyFRDataType();

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPyFRMergePointsFilter);

//----------------------------------------------------------------------------
vtkPyFRMergePointsFilter::vtkPyFRMergePointsFilter()
{
}

//----------------------------------------------------------------------------
vtkPyFRMergePointsFilter::~vtkPyFRMergePointsFilter()
{
}

//----------------------------------------------------------------------------
void vtkPyFRMergePointsFilter::SetInputData(vtkDataObject* input)
{
  this->SetInputData(0, input);
}

//----------------------------------------------------------------------------
void vtkPyFRMergePointsFilter::SetInputData(int index, vtkDataObject* input)
{
  this->SetInputDataInternal(index, input);
}

//----------------------------------------------------------------------------
int vtkPyFRMergePointsFilter::RequestData(
  vtkInformation*,
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkPyFRData *input = vtkPyFRData::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkPyFRData *output = vtkPyFRData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  PyFRMergePointsFilter filter;
  filter(input->GetData(),output->GetData());
  output->Modified();

  return 1;
}
//----------------------------------------------------------------------------

int vtkPyFRMergePointsFilter::FillInputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPyFRData");
  return 1;
}
//-----------------------------------------------------------------------------

void vtkPyFRMergePointsFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
