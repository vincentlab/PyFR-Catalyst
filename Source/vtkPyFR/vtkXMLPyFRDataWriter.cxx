#include <cmath>
#include <iomanip>
#include <string>
#include <sstream>
#include <stdexcept>

#include "vtkXMLPyFRDataWriter.h"

#include <vtkCellArray.h>
#include <vtkCellType.h>
#include <vtkCommand.h>
#include <vtkCommunicator.h>
#include <vtkCompleteArrays.h>
#include <vtkCPDataDescription.h>
#include <vtkCPInputDataDescription.h>
#include <vtkDataArray.h>
#include <vtkDoubleArray.h>
#include "vtkErrorCode.h"
#include "vtkExecutive.h"
#include <vtkFloatArray.h>
#include <vtkHexahedron.h>
#include <vtkIdTypeArray.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkPointData.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>
#include <vtkSmartPointer.h>
#include <vtkUnstructuredGrid.h>
#include <vtkXMLPolyDataWriter.h>
#include <vtkXMLUnstructuredGridWriter.h>

#include "vtkPyFRData.h"

#include "PyFRWriter.h"

vtkStandardNewMacro(vtkXMLPyFRDataWriter);

//----------------------------------------------------------------------------
vtkXMLPyFRDataWriter::vtkXMLPyFRDataWriter() : IsBinary(true),
                                               FileName("output.vtu")
{
}

//----------------------------------------------------------------------------
vtkXMLPyFRDataWriter::~vtkXMLPyFRDataWriter()
{
}

//----------------------------------------------------------------------------
void vtkXMLPyFRDataWriter::SetInputData(vtkDataObject* input)
{
  this->SetInputData(0, input);
}

//----------------------------------------------------------------------------
void vtkXMLPyFRDataWriter::SetInputData(int index, vtkDataObject* input)
{
  this->SetInputDataInternal(index, input);
}

//----------------------------------------------------------------------------
int vtkXMLPyFRDataWriter::Write()
{
  // Make sure we have input.
  if (this->GetNumberOfInputConnections(0) < 1)
    {
    vtkErrorMacro("No input provided!");
    return 0;
    }

  // always write even if the data hasn't changed
  this->Modified();
  this->UpdateWholeExtent();

  return (this->GetErrorCode() == vtkErrorCode::NoError);
}

//----------------------------------------------------------------------------
void vtkXMLPyFRDataWriter::WriteData()
{
  vtkPyFRData* pyfrData = vtkPyFRData::SafeDownCast(this->GetExecutive()->GetInputData(0, 0));
  if(!pyfrData)
    throw std::runtime_error("PyFRData input required.");

  PyFRWriter writer;
  writer.SetFileName(this->FileName);
  if (this->IsBinary)
    writer.SetDataModeToBinary();
  else
    writer.SetDataModeToAscii();

  writer(pyfrData->GetData());
}

//----------------------------------------------------------------------------
int vtkXMLPyFRDataWriter::RequestData(
  vtkInformation *,
  vtkInformationVector **,
  vtkInformationVector *)
{
  this->SetErrorCode(vtkErrorCode::NoError);

  vtkDataObject *input = this->GetInput();
  int idx;

  // make sure input is available
  if ( !input )
    {
    vtkErrorMacro(<< "No input!");
    return 0;
    }

  for (idx = 0; idx < this->GetNumberOfInputPorts(); ++idx)
    {
    if (this->GetInputExecutive(idx, 0) != NULL)
      {
      this->GetInputExecutive(idx, 0)->Update();
      }
    }

  this->InvokeEvent(vtkCommand::StartEvent,NULL);
  this->WriteData();
  this->InvokeEvent(vtkCommand::EndEvent,NULL);

  return 1;
}
//----------------------------------------------------------------------------

void vtkXMLPyFRDataWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "FileName: " << this->FileName << "\n";
}
