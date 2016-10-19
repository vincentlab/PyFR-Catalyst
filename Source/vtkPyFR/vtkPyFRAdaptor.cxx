#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string>
#include "vtkPyFRAdaptor.h"

#include <vtkCPDataDescription.h>
#include <vtkCPInputDataDescription.h>
#include <vtkCPProcessor.h>
#include <vtkMPICommunicator.h>
#include <vtkNew.h>
#include <vtkSmartPointer.h>
#include <vtkSMSourceProxy.h>

#include <vtkProcessModule.h>
#include <vtkPVOptions.h>

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

#define root(stmt) \
  do { \
    vtkMPICommunicator* comm = vtkMPICommunicator::GetWorldCommunicator(); \
    const vtkIdType rank = comm->GetLocalProcessId(); \
    if(0 == rank) { \
      stmt; \
    } \
  } while(0)

//----------------------------------------------------------------------------
void* CatalystInitialize(char* hostName, int pyport, char* outputfile, void* p)
{
  vtkPyFRData* data = vtkPyFRData::New();
  data->GetData()->Init(p);
  const char* envHost = getenv("PV_HOSTNAME");
  const char* envPortS = getenv("PV_PORT");
  const char* host = hostName;
  int port = pyport;
  if(NULL != envHost) {
    root(std::cout << "[catalyst] Overriding config file host with env var.\n");
    host = envHost;
  }
  if(NULL != envPortS) {
    root(std::cout << "[catalyst] Overriding config file port with env var.\n");
    port = atoi(envPortS);
  }
  root(std::cout << "[catalyst] host=" << host << ":" << port << "\n");
  assert(0 < port && port < 65536);

  if(Processor == NULL)
    {
    Processor = vtkCPProcessor::New();
    Processor->Initialize();

    //grab the vtkPVOptions and set the tile display parameters now
    //that Initialize has occurred and the options have been created.
    // vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
    // vtkPVOptions* options = pm->GetOptions();
    // options->SetTileDimensions(tileX,tileY);
    }
  vtkNew<vtkCPDataDescription> dataDescription;
  dataDescription->AddInput("input");
  dataDescription->SetTimeData(0, 0);
  dataDescription->GetInputDescriptionByName("input")->SetGrid(data);

  vtkNew<vtkPyFRPipeline> pipeline;
  pipeline->Initialize(host, port, outputfile, dataDescription.GetPointer());
  Processor->AddPipeline(pipeline.GetPointer());

  return data;
}

//----------------------------------------------------------------------------
void CatalystCamera(void* p, const float eye[3], const float ref[3],
               const float vup[3])
{
  vtkPyFRData* data = static_cast<vtkPyFRData*>(p);
  std::copy(eye, eye+3, data->GetData()->eye);
  std::copy(ref, ref+3, data->GetData()->ref);
  std::copy(vup, vup+3, data->GetData()->vup);
}

/// Sets the background color for the render window.
/// @param color 3-tuple of RGB color values, each in [0,1].
//----------------------------------------------------------------------------
void CatalystBGColor(void* p, const float color[3])
{
  vtkPyFRData* data = static_cast<vtkPyFRData*>(p);
  std::copy(color, color+3, data->GetData()->bg_color);
}

//----------------------------------------------------------------------------
void CatalystImageResolution(void*, const uint32_t imgsz[2])
{
  if(Processor == NULL)
    {
    fprintf(stderr, "%s: Catalyst not initialized!\n", __FUNCTION__);
    return;
    }
  vtkPyFRPipeline* pipeline =
    vtkPyFRPipeline::SafeDownCast(Processor->GetPipeline(0));
  pipeline->SetResolution(imgsz[0], imgsz[1]);
}

//----------------------------------------------------------------------------
void CatalystFinalize(void* p)
{
  vtkPyFRData* data = static_cast<vtkPyFRData*>(p);
  vtkPyFRPipeline* pipeline =
    vtkPyFRPipeline::SafeDownCast(Processor->GetPipeline(0));

  root(printf("[catalyst] finalizing %p at %s:%d\n", p, __FILE__, __LINE__));

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
  root(printf("[catalyst] coprocessing(%lf, %u, %p, %d) at %s:%d\n", time,
              timeStep, p, (int)lastTimeStep, __FILE__, __LINE__));
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
