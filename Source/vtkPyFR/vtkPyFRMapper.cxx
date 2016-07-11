
#include "vtkPyFRMapper.h"
#include "vtkPyFRContourMapper.h"

#include "vtkExecutive.h"
#include "vtkDemandDrivenPipeline.h"
#include "vtkObjectFactory.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkPolyData.h"
#include "vtkRenderWindow.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include <vtkPyFRContourData.h>

#include <vector>

vtkStandardNewMacro(vtkPyFRMapper);

class vtkPyFRMapperInternals
{
public:
  std::vector<vtkPyFRContourMapper*> Mappers;
};

//----------------------------------------------------------------------------
vtkPyFRMapper::vtkPyFRMapper()
{
  this->Internal = new vtkPyFRMapperInternals;
}

//----------------------------------------------------------------------------
vtkPyFRMapper::~vtkPyFRMapper()
{
  for(unsigned int i=0;i<this->Internal->Mappers.size();i++)
    {
    this->Internal->Mappers[i]->UnRegister(this);
    }
  this->Internal->Mappers.clear();

  delete this->Internal;
}

// Specify the type of data this mapper can handle.
//----------------------------------------------------------------------------
int vtkPyFRMapper::FillInputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPyFRContourData");
  return 1;
}

// When the structure is out-of-date, recreate it by
// creating a mapper for each input data.
//----------------------------------------------------------------------------
void vtkPyFRMapper::BuildMappers()
{
  //Get the dataset from the input
  vtkInformation* inInfo = this->GetExecutive()->GetInputInformation(0,0);
  vtkPyFRContourData *input = vtkPyFRContourData::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  int numContours = input->GetNumberOfContours();


  for (int i=0;i<numContours;i++)
    {
    if (i == this->Internal->Mappers.size())
      {
      vtkPyFRContourMapper *cmapper = vtkPyFRContourMapper::New();
      cmapper->Register(this); //increments the ref count on cmapper
      cmapper->SetInputData(input);
      this->Internal->Mappers.push_back(cmapper);

      cmapper->FastDelete();
      }

    this->Internal->Mappers[i]->SetActiveContour(input->HasData(i) ? i : -1);
    this->Internal->Mappers[i]->Modified();
    }

  for (int i=numContours;i<this->Internal->Mappers.size();i++)
    {
    this->Internal->Mappers[i]->SetActiveContour(-1);
    }

  unsigned nContours = 0;

  for (unsigned i=0;i<this->Internal->Mappers.size();i++)
    if (this->Internal->Mappers[i]->GetActiveContour() !=-1)
      nContours++;

  this->InternalMappersBuildTime.Modified();
}

//----------------------------------------------------------------------------
void vtkPyFRMapper::Render(vtkRenderer *ren, vtkActor *a)
{
  vtkDemandDrivenPipeline * executive =
  vtkDemandDrivenPipeline::SafeDownCast(this->GetExecutive());

  if(executive->GetPipelineMTime() >
     this->InternalMappersBuildTime.GetMTime())
    {
    this->BuildMappers();
    }

  this->TimeToDraw = 0;
  //Call Render() on each of the PolyDataMappers
  for(unsigned int i=0;i<this->Internal->Mappers.size();i++)
    {
    if ( this->ClippingPlanes !=
         this->Internal->Mappers[i]->GetClippingPlanes() )
      {
      this->Internal->Mappers[i]->SetClippingPlanes( this->ClippingPlanes );
      }

    this->Internal->Mappers[i]->Render(ren,a);
    this->TimeToDraw += this->Internal->Mappers[i]->GetTimeToDraw();
    }
}


//Looks at each DataSet and finds the union of all the bounds
//----------------------------------------------------------------------------
void vtkPyFRMapper::ComputeBounds()
{
  vtkMath::UninitializeBounds(this->Bounds);

  vtkInformation* inInfo = this->GetExecutive()->GetInputInformation(0,0);
  vtkPyFRContourData *input = vtkPyFRContourData::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));

  input->GetBounds( this->Bounds );
  this->BoundsMTime.Modified();
}

//----------------------------------------------------------------------------
double *vtkPyFRMapper::GetBounds()
{
  if ( ! this->GetExecutive()->GetInputData(0, 0) )
    {
    vtkMath::UninitializeBounds(this->Bounds);
    return this->Bounds;
    }
  else
    {
    this->Update();
    //only compute bounds when the input data has changed

    vtkDemandDrivenPipeline * executive =
    vtkDemandDrivenPipeline::SafeDownCast(this->GetExecutive());

    if(executive->GetPipelineMTime() >
        this->InternalMappersBuildTime.GetMTime())
      {
      this->ComputeBounds();
      }

    return this->Bounds;
    }
}

//----------------------------------------------------------------------------
void vtkPyFRMapper::ReleaseGraphicsResources( vtkWindow *win )
{
  for(unsigned int i=0;i<this->Internal->Mappers.size();i++)
    {
    this->Internal->Mappers[i]->ReleaseGraphicsResources( win );
    }
}

//----------------------------------------------------------------------------
void vtkPyFRMapper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
