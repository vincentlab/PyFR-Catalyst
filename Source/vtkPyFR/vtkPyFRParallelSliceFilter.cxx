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
                                                           NumberOfPlanes(1)
{
  this->Origin[0] = this->Origin[1] = this->Origin[2] = 0.;
  this->Normal[1] = this->Normal[2] = 0.;
  this->Normal[0] = 1.;

  //needs syncing since default color table is marked as using contour pipeline
  this->ColorPaletteNeedsSyncing = false;
  this->ColorPalette = 1;
  this->ColorRange[0] = 0.;
  this->ColorRange[1] = 1.;
}

//----------------------------------------------------------------------------
vtkPyFRParallelSliceFilter::~vtkPyFRParallelSliceFilter()
{

}

//----------------------------------------------------------------------------
void vtkPyFRParallelSliceFilter::SetColorPalette(int palette)
{
  if(palette != this->ColorPalette)
  {
    std::cout << "vtkPyFRParallelSliceFilter::SetColorPalette: " << palette << std::endl;
    this->Modified();
    this->ColorPaletteNeedsSyncing = true;
    this->ColorPalette = palette;
  }
}

//----------------------------------------------------------------------------
void vtkPyFRParallelSliceFilter::SetColorRange(double start, double end)
{
  if(start != this->ColorRange[0] || end != this->ColorRange[1])
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

  PyFRParallelSliceFilter filter;
  filter.SetPlane(this->Origin[0],this->Origin[1],this->Origin[2],
                   this->Normal[0],this->Normal[1],this->Normal[2]);
  filter.SetSpacing(this->Spacing);
  filter.SetNumberOfPlanes(this->NumberOfPlanes);
  filter(input->GetData(),output->GetData());

  if(this->ColorPaletteNeedsSyncing)
    {
    output->SetColorPalette(2, this->ColorPalette,this->ColorRange);
    this->ColorPaletteNeedsSyncing = false;
    }

  filter.MapFieldOntoSlices(this->MappedField,
                            input->GetData(),
                            output->GetData());
  output->Modified();
  return 1;
}

//----------------------------------------------------------------------------
void vtkPyFRParallelSliceFilter::SetMappedField(int i)
{
  if (this->MappedField != i)
    {
    std::cout << "vtkPyFRParallelSliceFilter Setting field to color by : " << i << std::endl;
    this->MappedField = i;
    this->Modified();
    this->ColorPaletteNeedsSyncing = true;
    }
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
