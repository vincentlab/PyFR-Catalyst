#include "vtkPyFRContourDataConverter.h"

#include <vtkDataObject.h>
#include <vtkDataObjectTypes.h>
#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkInstantiator.h>
#include <vtkObjectFactory.h>
#include <vtkPolyData.h>

#include "PyFRConverter.h"

#include "vtkPyFRContourData.h"

//----------------------------------------------------------------------------
int vtkPyFRContourDataConverter::RegisterPyFRDataType()
{
  vtkInstantiator::RegisterInstantiator("vtkPyFRContourData",
                                        &New_vtkPyFRContourData);
  return 1;
}

int vtkPyFRContourDataConverter::PyFRDataTypeRegistered =
  vtkPyFRContourDataConverter::RegisterPyFRDataType();

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPyFRContourDataConverter);

//----------------------------------------------------------------------------
vtkPyFRContourDataConverter::vtkPyFRContourDataConverter()
{
}

//----------------------------------------------------------------------------
vtkPyFRContourDataConverter::~vtkPyFRContourDataConverter()
{
}

//----------------------------------------------------------------------------
int vtkPyFRContourDataConverter::RequestDataObject(
  vtkInformation*,
  vtkInformationVector**,
  vtkInformationVector* outputVector)
{
  // for each output
  for(int i=0; i < this->GetNumberOfOutputPorts(); ++i)
    {
    vtkInformation* info = outputVector->GetInformationObject(i);
    vtkDataObject *output = info->Get(vtkDataObject::DATA_OBJECT());
    if (!output || !output->IsA("vtkPolyData"))
      {
      vtkDataObject* newOutput =
        vtkDataObjectTypes::NewDataObject(VTK_POLY_DATA);
      if (!newOutput)
        {
        vtkErrorMacro("Could not create output data type vtkPolyData");
        return 0;
        }
      this->GetOutputPortInformation(0)->Set(
        vtkDataObject::DATA_EXTENT_TYPE(), newOutput->GetExtentType());
      newOutput->Delete();
      }
    }

  return 1;
}

//----------------------------------------------------------------------------
int vtkPyFRContourDataConverter::RequestData(
  vtkInformation*,
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkPyFRContourData *input = vtkPyFRContourData::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkPolyData *output = vtkPolyData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  PyFRConverter convert;
  convert(input->GetData(),output);

  return 1;
}

//----------------------------------------------------------------------------
int vtkPyFRContourDataConverter::FillOutputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkPolyData");
  return 1;
}

//----------------------------------------------------------------------------
void vtkPyFRContourDataConverter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
