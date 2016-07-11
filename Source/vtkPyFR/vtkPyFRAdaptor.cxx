#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string>
#include "vtkPyFRAdaptor.h"

#include <vtkCPDataDescription.h>
#include <vtkCPInputDataDescription.h>
#include <vtkCPProcessor.h>
#include <vtkNew.h>
#include <vtkSmartPointer.h>
#include <vtkSMSourceProxy.h>

#include "PyFRData.h"

#include "vtkPyFRPipeline.h"
#include "vtkPyFRData.h"
#include "vtkPyFRContourData.h"
#include "vtkPyFRContourFilter.h"
#include "vtkPyFRParallelSliceFilter.h"

namespace
{
  vtkCPProcessor* Processor = NULL;
}

//----------------------------------------------------------------------------
void* CatalystInitialize(char* hostName, int port, char* outputfile, void* p)
{
  vtkPyFRData* data = vtkPyFRData::New();
  data->GetData()->Init(p);
  char* envHost = getenv("PV_HOSTNAME");
  if(NULL == envHost) {
    std::cerr << "PV_HOSTNAME environment variable is not set.  Bailing.\n";
    exit(EXIT_FAILURE);
  }
  const char* envPortS = getenv("PV_PORT");
  if(NULL == envPortS) {
    std::cerr << "PV_PORT not set.  Bailing.\n";
    exit(EXIT_FAILURE);
  }
  int envPort = atoi(envPortS ? envPortS : 0);
  std::cout << "[tjfCatalyst] ignoring '" << hostName << ":" << port << "'"
            << " and using '" << envHost << ":" << envPort << "' instead.\n";

  if(Processor == NULL)
    {
    Processor = vtkCPProcessor::New();
    Processor->Initialize();
    }
  vtkNew<vtkCPDataDescription> dataDescription;
  dataDescription->AddInput("input");
  dataDescription->SetTimeData(0, 0);
  dataDescription->GetInputDescriptionByName("input")->SetGrid(data);

  vtkNew<vtkPyFRPipeline> pipeline;
  pipeline->Initialize(envHost,envPort,outputfile,dataDescription.GetPointer());
  Processor->AddPipeline(pipeline.GetPointer());

  return data;
}

//----------------------------------------------------------------------------
void CatalystFinalize(void* p)
{
  vtkPyFRData* data = static_cast<vtkPyFRData*>(p);
  vtkPyFRPipeline* pipeline =
    vtkPyFRPipeline::SafeDownCast(Processor->GetPipeline(0));

  printf("[tjfCatalyst] finalizing %p at %s:%d\n", p, __FILE__, __LINE__);

  { //release contour gpu memory
  vtkObjectBase* clientSideContourBase = pipeline->GetContour()->GetClientSideObject();
  vtkPyFRContourFilter* realContour =
    vtkPyFRContourFilter::SafeDownCast(clientSideContourBase);
  vtkPyFRContourData* cData = vtkPyFRContourData::SafeDownCast(realContour->GetOutput());
  cData->ReleaseResources();
  }

  { //release slice gpu memory
  vtkObjectBase* clientSideSliceBase = pipeline->GetSlice()->GetClientSideObject();
  vtkPyFRParallelSliceFilter* realSlice =
    vtkPyFRParallelSliceFilter::SafeDownCast(clientSideSliceBase);
  vtkPyFRContourData* sData = vtkPyFRContourData::SafeDownCast(realSlice->GetOutput());
  sData->ReleaseResources();
  }

  if(Processor)
    {
    Processor->Delete();
    Processor = NULL;
    }
  if (data)
    {
    data->Delete();
    }
}

//----------------------------------------------------------------------------
void CatalystCoProcess(double time,unsigned int timeStep, void* p,bool lastTimeStep)
{
  vtkPyFRData* data = static_cast<vtkPyFRData*>(p);
  data->GetData()->Update();
  vtkNew<vtkCPDataDescription> dataDescription;
  dataDescription->AddInput("input");
  dataDescription->SetTimeData(time, timeStep);
#ifdef TJF_DEBUG
  printf("[tjfCatalyst] coprocessing(%lf, %u, %p, %d) at %s:%d\n", time, timeStep, p,
         (int)lastTimeStep, __FILE__, __LINE__);
#endif
  if(lastTimeStep == true)
    {
    // assume that we want to all the pipelines to execute if it
    // is the last time step.
    dataDescription->ForceOutputOn();
    }
  if(Processor->RequestDataDescription(dataDescription.GetPointer()) != 0)
    {
    dataDescription->GetInputDescriptionByName("input")->SetGrid(data);
    Processor->CoProcess(dataDescription.GetPointer());
    }
}
