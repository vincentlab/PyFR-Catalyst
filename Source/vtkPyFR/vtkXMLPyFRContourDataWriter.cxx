#include <cmath>
#include <iomanip>
#include <string>
#include <sstream>
#include <stdexcept>

#include "vtkXMLPyFRContourDataWriter.h"

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

#include "vtkPyFRContourData.h"

#include "PyFRWriter.h"

vtkStandardNewMacro(vtkXMLPyFRContourDataWriter);

//----------------------------------------------------------------------------
vtkXMLPyFRContourDataWriter::vtkXMLPyFRContourDataWriter() : IsBinary(true),
                                                             FileName("output")
{
}

//----------------------------------------------------------------------------
vtkXMLPyFRContourDataWriter::~vtkXMLPyFRContourDataWriter()
{
}

//----------------------------------------------------------------------------
void vtkXMLPyFRContourDataWriter::SetInputData(vtkDataObject* input)
{
  this->SetInputData(0, input);
}

//----------------------------------------------------------------------------
void vtkXMLPyFRContourDataWriter::SetInputData(int index, vtkDataObject* input)
{
  this->SetInputDataInternal(index, input);
}

//----------------------------------------------------------------------------
int vtkXMLPyFRContourDataWriter::Write()
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
void vtkXMLPyFRContourDataWriter::WriteData()
{
  vtkPyFRContourData* pyfrContourData =
    vtkPyFRContourData::SafeDownCast(this->GetExecutive()->GetInputData(0, 0));
  if(!pyfrContourData)
    throw std::runtime_error("PyFRContourData input required.");

  PyFRWriter writer;
  writer.SetFileName(this->FileName);
  if (this->IsBinary)
    writer.SetDataModeToBinary();
  else
    writer.SetDataModeToAscii();

  writer(pyfrContourData->GetData());
}

//----------------------------------------------------------------------------
int vtkXMLPyFRContourDataWriter::RequestData(
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

void vtkXMLPyFRContourDataWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "FileName: " << this->FileName << "\n";
}
