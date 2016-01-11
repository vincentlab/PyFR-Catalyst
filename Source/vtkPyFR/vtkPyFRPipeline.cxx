#include "vtkPyFRPipeline.h"

#include <sstream>

#include <vtkActor.h>
#include <vtkCollection.h>
#include <vtkCPDataDescription.h>
#include <vtkCPInputDataDescription.h>
#include <vtkDataSetMapper.h>
#include <vtkLiveInsituLink.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkPlane.h>
#include <vtkPolyDataMapper.h>
#include <vtkPVArrayInformation.h>
#include <vtkPVLiveRenderView.h>
#include <vtkPVTrivialProducer.h>
#include <vtkRenderer.h>
#include <vtkSmartPointer.h>
#include <vtkSMDoubleVectorProperty.h>
#include <vtkSMInputProperty.h>
#include <vtkSMIntVectorProperty.h>
#include <vtkSMOutputPort.h>
#include <vtkSMParaViewPipelineControllerWithRendering.h>
#include <vtkSMPluginManager.h>
#include <vtkSMPropertyHelper.h>
#include <vtkSMProxyListDomain.h>
#include <vtkSMProxyManager.h>
#include <vtkSMPVRepresentationProxy.h>
#include <vtkSMRenderViewProxy.h>
#include <vtkSMRepresentationProxy.h>
#include <vtkSMSessionProxyManager.h>
#include <vtkSMSourceProxy.h>
#include <vtkSMStringVectorProperty.h>
#include <vtkSMTransferFunctionManager.h>
#include <vtkSMTransferFunctionProxy.h>
#include <vtkSMViewProxy.h>
#include <vtkSMWriterProxy.h>
#include <vtkSTLReader.h>
#include <vtkTextActor.h>
#include <vtkTextProperty.h>
#include <vtkTextWidget.h>
#include <vtkTextRepresentation.h>
#include <vtkXMLUnstructuredGridReader.h>

#include "vtkPyFRData.h"
#include "vtkPyFRCrinkleClipFilter.h"
#include "vtkPyFRContourData.h"
#include "vtkPyFRContourFilter.h"
#include "vtkPyFRMapper.h"
#include "vtkPyFRParallelSliceFilter.h"
#include "vtkXMLPyFRDataWriter.h"
#include "vtkXMLPyFRContourDataWriter.h"
#include "vtkPVPlugin.h"

#include "PyFRData.h"

#define STRINGIFY(s) TOSTRING(s)
#define TOSTRING(s) #s

#ifdef SINGLE
PV_PLUGIN_IMPORT_INIT(pyfr_plugin_fp32)
#else
PV_PLUGIN_IMPORT_INIT(pyfr_plugin_fp64)
#endif

template <class Mapper>
void vtkAddActor(vtkSmartPointer<Mapper> mapper,
                 vtkSMSourceProxy* filter,
                 vtkSMViewProxy* view)
{
  vtkSMRenderViewProxy* rview = vtkSMRenderViewProxy::SafeDownCast(view);
  rview->UpdateVTKObjects();
  filter->UpdateVTKObjects();

  vtkRenderer* ren = rview->GetRenderer();
  vtkAlgorithm* algo = vtkAlgorithm::SafeDownCast(filter->GetClientSideObject());

  mapper->SetInputConnection(algo->GetOutputPort());

  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);

  ren->AddActor(actor.GetPointer());
}

void vtkUpdateFilter(vtkSMSourceProxy* filter, double time)
{
  filter->UpdatePipeline(time);
}

vtkStandardNewMacro(vtkPyFRPipeline);

//----------------------------------------------------------------------------
vtkPyFRPipeline::vtkPyFRPipeline() : InsituLink(NULL)
{
}

//----------------------------------------------------------------------------
vtkPyFRPipeline::~vtkPyFRPipeline()
{
  if (this->InsituLink)
    this->InsituLink->Delete();
}

//----------------------------------------------------------------------------
void vtkPyFRPipeline::Initialize(char* hostName, int port, char* fileName,
                                 vtkCPDataDescription* dataDescription)
{
  this->FileName = std::string(fileName);

  vtkSMProxyManager* proxyManager = vtkSMProxyManager::GetProxyManager();

  // Load PyFR plugin
#ifdef SINGLE
PV_PLUGIN_IMPORT(pyfr_plugin_fp32)
#else
PV_PLUGIN_IMPORT(pyfr_plugin_fp64)
#endif

  // Grab the active session proxy manager
  vtkSMSessionProxyManager* sessionProxyManager =
    proxyManager->GetActiveSessionProxyManager();

  // Create the vtkLiveInsituLink (the "link" to the visualization processes).
  this->InsituLink = vtkLiveInsituLink::New();

  // Tell vtkLiveInsituLink what host/port must it connect to for the
  // visualization process.
  this->InsituLink->SetHostname(hostName);
  this->InsituLink->SetInsituPort(port);

  // Grab the data object from the data description
  vtkPyFRData* pyfrData =
    vtkPyFRData::SafeDownCast(dataDescription->
                              GetInputDescriptionByName("input")->GetGrid());

  // If these flags are set to true, the data will be written to vtk files on
  // the server side, but the pipeline cannot be cannected to a client.
  bool preFilterWrite = false;
  bool postFilterWrite = false;

  // Construct a pipeline controller to register my elements
  vtkNew<vtkSMParaViewPipelineControllerWithRendering> controller;

  // Create a vtkPVTrivialProducer and set its output
  // to be the input data.
  vtkSmartPointer<vtkSMSourceProxy> producer;
  producer.TakeReference(
    vtkSMSourceProxy::SafeDownCast(
      sessionProxyManager->NewProxy("sources", "PVTrivialProducer")));
  producer->UpdateVTKObjects();
  vtkObjectBase* clientSideObject = producer->GetClientSideObject();
  vtkPVTrivialProducer* realProducer =
    vtkPVTrivialProducer::SafeDownCast(clientSideObject);
  realProducer->SetOutput(pyfrData);
  controller->InitializeProxy(producer);
  controller->RegisterPipelineProxy(producer,"Source");

  if (preFilterWrite)
    {
    // Create a convertor to convert the pyfr data into a vtkUnstructuredGrid
    vtkSmartPointer<vtkSMSourceProxy> pyfrDataConverter;
    pyfrDataConverter.TakeReference(
      vtkSMSourceProxy::SafeDownCast(sessionProxyManager->
                                     NewProxy("filters", "PyFRDataConverter")));
    vtkSMInputProperty* pyfrDataConverterInputConnection =
      vtkSMInputProperty::SafeDownCast(pyfrDataConverter->GetProperty("Input"));

    producer->UpdateVTKObjects();
    pyfrDataConverterInputConnection->SetInputConnection(0, producer, 0);
    pyfrDataConverter->UpdatePropertyInformation();
    pyfrDataConverter->UpdateVTKObjects();
    controller->InitializeProxy(pyfrDataConverter);
    controller->RegisterPipelineProxy(pyfrDataConverter,"convertPyFRData");

    // Create an unstructured grid writer, set the filename and then update the
    // pipeline.
    vtkSmartPointer<vtkSMWriterProxy> unstructuredGridWriter;
    unstructuredGridWriter.TakeReference(
      vtkSMWriterProxy::SafeDownCast(sessionProxyManager->
                                     NewProxy("writers",
                                              "XMLUnstructuredGridWriter")));
    vtkSMInputProperty* unstructuredGridWriterInputConnection =
      vtkSMInputProperty::SafeDownCast(unstructuredGridWriter->
                                       GetProperty("Input"));
    unstructuredGridWriterInputConnection->SetInputConnection(0,
                                                              pyfrDataConverter,
                                                              0);
    vtkSMStringVectorProperty* unstructuredGridFileName =
      vtkSMStringVectorProperty::SafeDownCast(unstructuredGridWriter->
                                              GetProperty("FileName"));

      {
      std::ostringstream o;
      o << this->FileName.substr(0,this->FileName.find_last_of("."));
      o << "_" <<std::fixed << std::setprecision(3)<<dataDescription->GetTime();
      o << ".vtu";
      unstructuredGridFileName->SetElement(0, o.str().c_str());
      }

      unstructuredGridWriter->UpdatePropertyInformation();
      unstructuredGridWriter->UpdateVTKObjects();
      unstructuredGridWriter->UpdatePipeline();
      controller->InitializeProxy(unstructuredGridWriter);
      controller->RegisterPipelineProxy(unstructuredGridWriter,
                                        "UnstructuredGridWriter");
    }

  // Add the clip filter
  this->Clip.TakeReference(
    vtkSMSourceProxy::SafeDownCast(sessionProxyManager->
                                   NewProxy("filters",
                                            "PyFRCrinkleClipFilter")));
  controller->PreInitializeProxy(this->Clip);
  vtkSMPropertyHelper(this->Clip, "Input").Set(producer, 0);
  this->Clip->UpdateVTKObjects();
  controller->PostInitializeProxy(this->Clip);
  controller->RegisterPipelineProxy(this->Clip,"Clip");

  // Add the slice filter
  this->Slice.TakeReference(
    vtkSMSourceProxy::SafeDownCast(sessionProxyManager->
                                   NewProxy("filters",
                                            "PyFRParallelSliceFilter")));
  controller->PreInitializeProxy(this->Slice);
  vtkSMPropertyHelper(this->Slice, "Input").Set(producer, 0);
  vtkSMPropertyHelper(this->Slice,"ColorField").Set(0);
  double sliceColorRange[2] = {0.695,0.7385};
  vtkSMPropertyHelper(this->Slice,"ColorRange").Set(sliceColorRange,2);
  this->Slice->UpdateVTKObjects();
  controller->PostInitializeProxy(this->Slice);
  controller->RegisterPipelineProxy(this->Slice,"Slice");

  // Add the contour filter
  this->Contour.TakeReference(
    vtkSMSourceProxy::SafeDownCast(sessionProxyManager->
                                   NewProxy("filters",
                                            "PyFRContourFilter")));
  vtkSMInputProperty* contourInputConnection =
    vtkSMInputProperty::SafeDownCast(this->Contour->GetProperty("Input"));

  controller->PreInitializeProxy(this->Contour);
  vtkSMPropertyHelper(this->Contour, "Input").Set(this->Clip, 0);
  vtkSMPropertyHelper(this->Contour,"ContourField").Set(0);
  vtkSMPropertyHelper(this->Contour,"ColorField").Set(0);

  // Values for the PyFR demo
  vtkSMPropertyHelper(this->Contour,"ContourValues").Set(0,.738);
  vtkSMPropertyHelper(this->Contour,"ContourValues").Set(1,.7392);
  vtkSMPropertyHelper(this->Contour,"ContourValues").Set(2,.7404);
  vtkSMPropertyHelper(this->Contour,"ContourValues").Set(3,.7416);
  vtkSMPropertyHelper(this->Contour,"ContourValues").Set(4,.7428);
  double contourColorRange[2] = {.7377,.7428};
  vtkSMPropertyHelper(this->Contour,"ColorRange").Set(contourColorRange,2);

  this->Contour->UpdateVTKObjects();
  controller->PostInitializeProxy(this->Contour);
  controller->RegisterPipelineProxy(this->Contour,"Contour");

  // Create a view
  vtkSmartPointer<vtkSMViewProxy> polydataViewer;
  polydataViewer.TakeReference(
    vtkSMViewProxy::SafeDownCast(sessionProxyManager->
                                 NewProxy("views","RenderView")));
  controller->InitializeProxy(polydataViewer);
  controller->RegisterViewProxy(polydataViewer);

  // Show the results.
  this->ContourMapper = vtkSmartPointer<vtkPyFRMapper>::New();
  this->SliceMapper = vtkSmartPointer<vtkPyFRMapper>::New();
  vtkAddActor(this->ContourMapper, this->Contour, polydataViewer);
  vtkAddActor(this->SliceMapper, this->Slice, polydataViewer);

  if (postFilterWrite)
    {
    // Create a converter to convert the pyfr contour data into polydata
    vtkSmartPointer<vtkSMSourceProxy> pyfrContourDataConverter;
    pyfrContourDataConverter.TakeReference(
      vtkSMSourceProxy::SafeDownCast(sessionProxyManager->
                                     NewProxy("filters",
                                              "PyFRContourDataConverter")));
    vtkSMInputProperty* pyfrContourDataConverterInputConnection =
      vtkSMInputProperty::SafeDownCast(pyfrContourDataConverter->
                                       GetProperty("Input"));

    pyfrContourDataConverterInputConnection->SetInputConnection(0, this->Contour, 0);
    pyfrContourDataConverter->UpdatePropertyInformation();
    pyfrContourDataConverter->UpdateVTKObjects();
    controller->InitializeProxy(pyfrContourDataConverter);
    controller->RegisterPipelineProxy(pyfrContourDataConverter,
                                      "ConvertContoursToPolyData");

    // Create the polydata writer, set the filename and then update the pipeline
    vtkSmartPointer<vtkSMWriterProxy> polydataWriter;
    polydataWriter.TakeReference(
      vtkSMWriterProxy::SafeDownCast(sessionProxyManager->
                                     NewProxy("writers", "XMLPolyDataWriter")));
    vtkSMInputProperty* polydataWriterInputConnection =
      vtkSMInputProperty::SafeDownCast(polydataWriter->GetProperty("Input"));
    polydataWriterInputConnection->SetInputConnection(0,
                                                      pyfrContourDataConverter,
                                                      0);
    vtkSMStringVectorProperty* polydataFileName =
      vtkSMStringVectorProperty::SafeDownCast(polydataWriter->
                                              GetProperty("FileName"));

      {
      std::ostringstream o;
      o << this->FileName.substr(0,this->FileName.find_last_of("."));
      o << "_"<<std::fixed<<std::setprecision(3)<<dataDescription->GetTime();
      o << ".vtp";
      polydataFileName->SetElement(0, o.str().c_str());
      }

      polydataWriter->UpdatePropertyInformation();
      polydataWriter->UpdateVTKObjects();
      polydataWriter->UpdatePipeline();
      controller->InitializeProxy(polydataWriter);
      controller->RegisterPipelineProxy(polydataWriter,"polydataWriter");
    }

  vtkSmartPointer<vtkSMSourceProxy> airplane;
  airplane.TakeReference(
    vtkSMSourceProxy::SafeDownCast(sessionProxyManager->
                                   NewProxy("internal_sources",
                                            "XMLUnstructuredGridReaderCore")));
  controller->PreInitializeProxy(airplane);
    {
    std::ostringstream o;
    o << STRINGIFY(DATA_DIR) << "/airplane.vtu";
    vtkSMPropertyHelper(airplane, "FileName").Set(o.str().c_str());
    }
  airplane->UpdateVTKObjects();
  controller->PostInitializeProxy(airplane);
  controller->RegisterPipelineProxy(airplane,"Airplane");

  // Create a view
  vtkSmartPointer<vtkDataSetMapper> airplaneMapper =
    vtkSmartPointer<vtkDataSetMapper>::New();
  vtkAddActor(airplaneMapper, airplane, polydataViewer);

  // Create a timestamp
  this->Timestamp = vtkSmartPointer<vtkTextActor>::New();
    {
    std::ostringstream o;
    o << dataDescription->GetTime()<<" s";
    // o << dataDescription->GetTimeStep();
    this->Timestamp->SetInput(o.str().c_str());
    }
  this->Timestamp->GetTextProperty()->SetBackgroundColor(0.,0.,0.);
  this->Timestamp->GetTextProperty()->SetBackgroundOpacity(1);

  // Add the actors to the renderer, set the background and size
    {
    vtkSMRenderViewProxy* rview = vtkSMRenderViewProxy::SafeDownCast(polydataViewer);

    vtkRenderer* ren = rview->GetRenderer();
    ren->AddActor2D(this->Timestamp);
    rview->UpdateVTKObjects();
    }

  // Initialize the "link"
  this->InsituLink->InsituInitialize(vtkSMProxyManager::GetProxyManager()->
                                     GetActiveSessionProxyManager());
}

//----------------------------------------------------------------------------
int vtkPyFRPipeline::RequestDataDescription(
  vtkCPDataDescription* dataDescription)
{
  if(!dataDescription)
    {
    vtkWarningMacro("dataDescription is NULL.");
    return 0;
    }

  if(this->FileName.empty())
    {
    vtkWarningMacro("No output file name given to output results to.");
    return 0;
    }

  dataDescription->GetInputDescriptionByName("input")->AllFieldsOn();
  dataDescription->GetInputDescriptionByName("input")->GenerateMeshOn();
  return 1;
}

//----------------------------------------------------------------------------
int vtkPyFRPipeline::CoProcess(vtkCPDataDescription* dataDescription)
{
  vtkSMSessionProxyManager* sessionProxyManager =
    vtkSMProxyManager::GetProxyManager()->GetActiveSessionProxyManager();

  // Grab the data object from the data description
  vtkPyFRData* pyfrData =
    vtkPyFRData::SafeDownCast(dataDescription->
                              GetInputDescriptionByName("input")->GetGrid());

  // Use it to update the source
  vtkSMSourceProxy* source =
    vtkSMSourceProxy::SafeDownCast(sessionProxyManager->GetProxy("Source"));
  vtkObjectBase* clientSideObject = source->GetClientSideObject();
  vtkPVTrivialProducer* realProducer =
    vtkPVTrivialProducer::SafeDownCast(clientSideObject);
  realProducer->SetOutput(pyfrData,dataDescription->GetTime());

    {
    std::ostringstream o;
    o << dataDescription->GetTime()<<" s";
    // o << dataDescription->GetTimeStep();
    this->Timestamp->SetInput(o.str().c_str());
    }

  vtkSMSourceProxy* unstructuredGridWriter =
    vtkSMSourceProxy::SafeDownCast(sessionProxyManager->
				   GetProxy("UnstructuredGridWriter"));
  if (unstructuredGridWriter)
    {
    vtkSMStringVectorProperty* unstructuredGridFileName =
      vtkSMStringVectorProperty::SafeDownCast(unstructuredGridWriter->
                                              GetProperty("FileName"));

      {
      std::ostringstream o;
      o << this->FileName.substr(0,this->FileName.find_last_of("."));
      o << "_"<<std::fixed<<std::setprecision(3)<<dataDescription->GetTime();
      o << ".vtu";
      unstructuredGridFileName->SetElement(0, o.str().c_str());
      }
      unstructuredGridWriter->UpdatePropertyInformation();
      unstructuredGridWriter->UpdateVTKObjects();
      unstructuredGridWriter->UpdatePipeline();
    }
  vtkSMSourceProxy* polydataWriter =
    vtkSMSourceProxy::SafeDownCast(sessionProxyManager->
				   GetProxy("polydataWriter"));
  if (polydataWriter)
    {
    vtkSMStringVectorProperty* polydataFileName =
      vtkSMStringVectorProperty::SafeDownCast(polydataWriter->
                                              GetProperty("FileName"));

      {
      std::ostringstream o;
      o << this->FileName.substr(0,this->FileName.find_last_of("."));
      o << "_"<<std::fixed<<std::setprecision(3)<<dataDescription->GetTime();
      o << ".vtp";
      polydataFileName->SetElement(0, o.str().c_str());
      }
      polydataWriter->UpdatePropertyInformation();
      polydataWriter->UpdateVTKObjects();
      polydataWriter->UpdatePipeline();
    }

  // stay in the loop while the simulation is paused
  while (true)
    {
    this->InsituLink->InsituUpdate(dataDescription->GetTime(),
                                   dataDescription->GetTimeStep());

    vtkUpdateFilter(this->Contour, dataDescription->GetTime());
    vtkUpdateFilter(this->Slice, dataDescription->GetTime());

    vtkNew<vtkCollection> views;
    sessionProxyManager->GetProxies("views",views.GetPointer());
    for (int i=0;i<views->GetNumberOfItems();i++)
      {
      vtkSMViewProxy* viewProxy =
        vtkSMViewProxy::SafeDownCast(views->GetItemAsObject(i));
      vtkSMPropertyHelper(viewProxy,"ViewTime").Set(dataDescription->GetTime());
      viewProxy->UpdateVTKObjects();
      viewProxy->Update();
      }

    this->InsituLink->InsituPostProcess(dataDescription->GetTime(),
                                        dataDescription->GetTimeStep());
    if (this->InsituLink->GetSimulationPaused())
      {
      if (this->InsituLink->WaitForLiveChange())
        {
        break;
        }
      }
    else
      {
      break;
      }
    }

  return 1;
}

//----------------------------------------------------------------------------
void vtkPyFRPipeline::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "FileName: " << this->FileName << "\n";
}
