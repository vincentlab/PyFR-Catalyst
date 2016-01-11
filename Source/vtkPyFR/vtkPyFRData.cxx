#include "vtkPyFRData.h"

#include <vtkDataObjectTypes.h>
#include <vtkObjectFactory.h>

#include "PyFRData.h"

vtkStandardNewMacro(vtkPyFRData);

//----------------------------------------------------------------------------
vtkPyFRData::vtkPyFRData()
{
  this->data = new PyFRData();
}

//----------------------------------------------------------------------------
vtkPyFRData::~vtkPyFRData()
{
  delete this->data;
}

//----------------------------------------------------------------------------
void vtkPyFRData::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

//----------------------------------------------------------------------------
vtkObject* New_vtkPyFRData() { return vtkPyFRData::New(); }
