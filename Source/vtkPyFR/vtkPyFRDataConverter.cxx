#include "vtkPyFRDataConverter.h"

#include <vtkDataObject.h>
#include <vtkDataObjectTypes.h>
#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkInstantiator.h>
#include <vtkObjectFactory.h>
#include <vtkUnstructuredGrid.h>

#include "PyFRConverter.h"

#include "vtkPyFRData.h"

//----------------------------------------------------------------------------
int vtkPyFRDataConverter::RegisterPyFRDataType()
{
  vtkInstantiator::RegisterInstantiator("vtkPyFRData",&New_vtkPyFRData);
  return 1;
}

int vtkPyFRDataConverter::PyFRDataTypeRegistered =
  vtkPyFRDataConverter::RegisterPyFRDataType();

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPyFRDataConverter);

//----------------------------------------------------------------------------
vtkPyFRDataConverter::vtkPyFRDataConverter()
{
}

//----------------------------------------------------------------------------
vtkPyFRDataConverter::~vtkPyFRDataConverter()
{
}

//----------------------------------------------------------------------------
int vtkPyFRDataConverter::RequestData(
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
  vtkUnstructuredGrid *output = vtkUnstructuredGrid::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  PyFRConverter convert;
  convert(input->GetData(),output);

  return 1;
}

//----------------------------------------------------------------------------
int vtkPyFRDataConverter::FillInputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPyFRData");
  return 1;
}

//----------------------------------------------------------------------------
void vtkPyFRDataConverter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
