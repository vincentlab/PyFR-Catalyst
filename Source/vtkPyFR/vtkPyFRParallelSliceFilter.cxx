#include "vtkPyFRParallelSliceFilter.h"

#include <vtkDataObject.h>
#include <vtkDataObjectTypes.h>
#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkInstantiator.h>
#include <vtkObjectFactory.h>

#include "PyFRParallelSliceFilter.h"

#include "vtkPyFRData.h"
#include "vtkPyFRContourData.h"

//----------------------------------------------------------------------------
int vtkPyFRParallelSliceFilter::RegisterPyFRDataTypes()
{
  vtkInstantiator::RegisterInstantiator("vtkPyFRData",
                                        &New_vtkPyFRData);
  vtkInstantiator::RegisterInstantiator("vtkPyFRContourData",
                                        &New_vtkPyFRContourData);

  return 1;
}

int vtkPyFRParallelSliceFilter::PyFRDataTypesRegistered =
  vtkPyFRParallelSliceFilter::RegisterPyFRDataTypes();

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPyFRParallelSliceFilter);

//----------------------------------------------------------------------------
vtkPyFRParallelSliceFilter::vtkPyFRParallelSliceFilter() : Spacing(1.),
                                                           NumberOfPlanes(1),
                                                           LastExecuteTime(0)
{
  this->Origin[0] = this->Origin[1] = this->Origin[2] = 0.;
  this->Normal[1] = this->Normal[2] = 0.;
  this->Normal[0] = 1.;

  this->ColorPaletteNeedsSyncing = false;
  this->ColorPalette = 0;
  this->ColorRange[0] = 0.;
  this->ColorRange[1] = 1.;
  this->Filter = new PyFRParallelSliceFilter();
}

//----------------------------------------------------------------------------
vtkPyFRParallelSliceFilter::~vtkPyFRParallelSliceFilter()
{
  delete this->Filter;
}

//----------------------------------------------------------------------------
void vtkPyFRParallelSliceFilter::SetColorPalette(int palette)
{
  if(palette != this->ColorPalette)
  {
    this->Modified();
    this->ColorPaletteNeedsSyncing = true;
    this->ColorPalette = palette;
  }
}

//----------------------------------------------------------------------------
void vtkPyFRParallelSliceFilter::SetColorRange(double start, double end)
{
  if(start != this->ColorRange[0] && end != this->ColorRange[1])
  {
    this->Modified();
    this->ColorPaletteNeedsSyncing = true;
    this->ColorRange[0] = start;
    this->ColorRange[1] = end;
  }
}

//----------------------------------------------------------------------------
int vtkPyFRParallelSliceFilter::RequestData(
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
  vtkPyFRContourData *output = vtkPyFRContourData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  if (this->GetMTime() > this->LastExecuteTime)
    {
    this->LastExecuteTime = this->GetMTime();
    Filter->SetPlane(this->Origin[0],this->Origin[1],this->Origin[2],
                     this->Normal[0],this->Normal[1],this->Normal[2]);
    Filter->SetSpacing(this->Spacing);
    Filter->SetNumberOfPlanes(this->NumberOfPlanes);
    Filter->operator()(input->GetData(),output->GetData());
    }

  if(this->ColorPaletteNeedsSyncing)
    {
    output->SetColorPalette(this->ColorPalette,this->ColorRange);
    this->ColorPaletteNeedsSyncing = false;
    }

  Filter->MapFieldOntoSlices(this->MappedField,input->GetData(),
                             output->GetData());
  output->Modified();

  return 1;
}
//----------------------------------------------------------------------------

int vtkPyFRParallelSliceFilter::FillInputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPyFRData");
  return 1;
}
//----------------------------------------------------------------------------

void vtkPyFRParallelSliceFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
