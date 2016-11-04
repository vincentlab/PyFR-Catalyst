#include "vtkPyFRCrinkleClipFilter.h"

#include <vtkDataObject.h>
#include <vtkDataObjectTypes.h>
#include "vtkImplicitFunction.h"
#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkInstantiator.h>
#include <vtkObjectFactory.h>
#include <vtkPlane.h>
#include <vtkNew.h>
#include "PyFRCrinkleClipFilter.h"

#include "vtkPyFRData.h"

//----------------------------------------------------------------------------
int vtkPyFRCrinkleClipFilter::RegisterPyFRDataType()
{
  vtkInstantiator::RegisterInstantiator("vtkPyFRData",
                                        &New_vtkPyFRData);
  return 1;
}

int vtkPyFRCrinkleClipFilter::PyFRDataTypeRegistered =
  vtkPyFRCrinkleClipFilter::RegisterPyFRDataType();

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPyFRCrinkleClipFilter);

//----------------------------------------------------------------------------
vtkPyFRCrinkleClipFilter::vtkPyFRCrinkleClipFilter() : LastExecuteTime(0)
{
}

//----------------------------------------------------------------------------
vtkPyFRCrinkleClipFilter::~vtkPyFRCrinkleClipFilter()
{
}

//----------------------------------------------------------------------------
void vtkPyFRCrinkleClipFilter::SetInputData(vtkDataObject* input)
{
  this->SetInputData(0, input);
}

//----------------------------------------------------------------------------
void vtkPyFRCrinkleClipFilter::SetInputData(int index, vtkDataObject* input)
{
  this->SetInputDataInternal(index, input);
}

//----------------------------------------------------------------------------
int vtkPyFRCrinkleClipFilter::RequestData(
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

  //if (this->GetMTime() > this->LastExecuteTime)
    {
    this->LastExecuteTime = this->GetMTime();
    PyFRCrinkleClipFilter filter;
    filter.SetPlane(this->Origin[0],this->Origin[1],this->Origin[2],
                    this->Normal[0],this->Normal[1],this->Normal[2]);
    std::cout << "Set Plane Origin: " << this->Origin[0] << " "
              << this->Origin[1] << " " << this->Origin[2] << std::endl;
    std::cout << "Set Plane Normal: " << this->Normal[0] << " "
              << this->Normal[1] << " " << this->Normal[2] << std::endl;
    filter(input->GetData(),output->GetData());
    output->Modified();
    }

  return 1;
}
//----------------------------------------------------------------------------

int vtkPyFRCrinkleClipFilter::FillInputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPyFRData");
  return 1;
}
//-----------------------------------------------------------------------------

void vtkPyFRCrinkleClipFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
