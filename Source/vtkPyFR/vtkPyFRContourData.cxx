#include "vtkPyFRContourData.h"

#include <vtkDataObjectTypes.h>
#include <vtkObjectFactory.h>

#include "PyFRContourData.h"

vtkStandardNewMacro(vtkPyFRContourData);

//----------------------------------------------------------------------------
vtkPyFRContourData::vtkPyFRContourData()
{
  this->data = new PyFRContourData();
  for (unsigned i=0;i<6;i++)
    {
    this->Bounds[i] = 0.;
    }
  this->ColorPalette = 2;
  this->ColorRange[0] = 0.;
  this->ColorRange[1] = 1.;
}

//----------------------------------------------------------------------------
vtkPyFRContourData::~vtkPyFRContourData()
{
}

//----------------------------------------------------------------------------
void vtkPyFRContourData::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

//----------------------------------------------------------------------------
void vtkPyFRContourData::ShallowCopy(vtkPyFRContourData* contourData)
{
  for (int i=0;i<6;i++)
    {
    this->Bounds[i] = contourData->GetBounds()[i];
    }
  this->ColorRange[0] = contourData->GetColorRange()[0];
  this->ColorRange[1] = contourData->GetColorRange()[1];
  this->data = contourData->GetData();
}

//----------------------------------------------------------------------------
int vtkPyFRContourData::GetNumberOfContours() const
{
  if (this->data)
    {
    return static_cast<int>(this->data->GetNumberOfContours());
    }
  return 0;
}

//----------------------------------------------------------------------------
std::size_t vtkPyFRContourData::GetSizeOfContour(int i) const
{
  if (this->data)
    {
    return this->data->GetContourSize(i);
    }
  return 0;
}

//----------------------------------------------------------------------------
void vtkPyFRContourData::ReleaseResources()
{
  if (this->data)
    {
    delete this->data;
    this->data = NULL;
    }
}

//----------------------------------------------------------------------------
void vtkPyFRContourData::GetBounds(double* bounds)
{
  FPType b[6];
  this->data->ComputeBounds(b);
  for(size_t i=0; i < 6; ++i) {
    bounds[i] = b[i];
  }

  // if (this->GetMTime() > this->BoundsUpdateTime)
  //   {
  //   this->BoundsUpdateTime = this->GetMTime();
  //   FPType b[6];
  //   this->data->ComputeBounds(b);
  //   for (unsigned i=0;i<6;i++)
  //     {
  //     this->Bounds[i] = b[i];
  //     }
  //   }
  // for (unsigned i=0;i<6;i++)
  //   {
  //   bounds[i] = this->Bounds[i];
  //   }
}

//----------------------------------------------------------------------------
const double* vtkPyFRContourData::GetBounds()
{
  // if (this->GetMTime() > this->BoundsUpdateTime)
  //   {
  //   this->BoundsUpdateTime = this->GetMTime();
  //   FPType b[6];
  //   this->data->ComputeBounds(b);
  //   for (unsigned i=0;i<6;i++)
  //     {
  //     this->Bounds[i] = b[i];
  //     }
  //   }
  this->GetBounds(this->Bounds);

  return this->Bounds;
}

//----------------------------------------------------------------------------
bool vtkPyFRContourData::HasData() const
{
  if (this->data)
    {
    for (unsigned i=0;i<this->data->GetNumberOfContours();i++)
      {
      if (this->HasData(i))
        return true;
      }
    }
  return false;
}

//----------------------------------------------------------------------------
bool vtkPyFRContourData::HasData(int i) const
{
  if (this->data)
    {
    return this->data->GetContourSize(i) > 0;
    }
  return false;
}

//----------------------------------------------------------------------------
void vtkPyFRContourData::SetColorPalette(int i,double* range)
{
  if (this->ColorPalette != i ||
      this->ColorRange[0] != range[0] || this->ColorRange[1] != range[1])
    {
    this->ColorPalette = i;
    this->ColorRange[0] = range[0];
    this->ColorRange[1] = range[1];
    this->data->SetColorPalette(this->ColorPalette,
                                this->ColorRange[0],this->ColorRange[1]);
    this->Modified();
    }
}

//----------------------------------------------------------------------------
vtkObject* New_vtkPyFRContourData() { return vtkPyFRContourData::New(); }
