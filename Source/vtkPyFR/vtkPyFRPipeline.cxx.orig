#include "vtkPyFRPipeline.h"

#include <sstream>

#include <vtkCollection.h>
#include <vtkCPDataDescription.h>
#include <vtkCPInputDataDescription.h>
#include <vtkLiveInsituLink.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkPVLiveRenderView.h>
#include <vtkPVTrivialProducer.h>
#include <vtkSmartPointer.h>
#include <vtkSMIntVectorProperty.h>
#include <vtkSMDoubleVectorProperty.h>
#include <vtkSMInputProperty.h>
#include <vtkSMOutputPort.h>
#include <vtkSMParaViewPipelineControllerWithRendering.h>
#include <vtkSMPluginManager.h>
#include <vtkSMPropertyHelper.h>
#include <vtkSMProxyManager.h>
#include <vtkSMPVRepresentationProxy.h>
#include <vtkSMRepresentationProxy.h>
#include <vtkSMSourceProxy.h>
#include <vtkSMSessionProxyManager.h>
#include <vtkSMStringVectorProperty.h>
#include <vtkSMViewProxy.h>
#include <vtkSMWriterProxy.h>

#include "vtkPyFRData.h"
#include "vtkPyFRContourData.h"
#include "vtkPyFRContourDataConverter.h"
#include "vtkPyFRContourFilter.h"
#include "vtkXMLPyFRDataWriter.h"
#include "vtkXMLPyFRContourDataWriter.h"

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

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
  proxyManager->GetPluginManager()->LoadLocalPlugin(TOSTRING(PyFRPlugin));

  // Grab the active session proxy manager
  vtkSMSessionProxyManager* sessionProxyManager =
    proxyManager->GetActiveSessionProxyManager();

  // Create the vtkLiveInsituLink (the "link" to the visualization processes).
  this->InsituLink = vtkLiveInsituLink::New();

  // Tell vtkLiveInsituLink what host/port must it connect to for the
  // visualization process.
  this->InsituLink->SetHostname(hostName);
  this->InsituLink->SetInsituPort(port);

  // Initialize the "link"
  this->InsituLink->InsituInitialize(vtkSMProxyManager::GetProxyManager()->
                                     GetActiveSessionProxyManager());

  // Grab the data object from the data description
  vtkPyFRData* pyfrData =
    vtkPyFRData::SafeDownCast(dataDescription->
                              GetInputDescriptionByName("input")->GetGrid());

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

  // Add the contour filter
  vtkSmartPointer<vtkSMSourceProxy> contour;
  contour.TakeReference(
    vtkSMSourceProxy::SafeDownCast(sessionProxyManager->
                                   NewProxy("filters",
                                            "PyFRContourFilter")));
  vtkSMInputProperty* contourInputConnection =
    vtkSMInputProperty::SafeDownCast(contour->GetProperty("Input"));

  vtkSMIntVectorProperty* contourField =
    vtkSMIntVectorProperty::SafeDownCast(contour->GetProperty("ContourField"));
  contourField->SetElements1(0);

  vtkSMDoubleVectorProperty* contourValue =
    vtkSMDoubleVectorProperty::SafeDownCast(contour->
                                            GetProperty("ContourValue"));
  contourValue->SetElements1(1.0045);

  contour->UpdateVTKObjects();
  contourInputConnection->SetInputConnection(0, producer, 0);
  contour->UpdatePropertyInformation();
  contour->UpdateVTKObjects();
  controller->InitializeProxy(contour);
  controller->RegisterPipelineProxy(contour,"Contour");

  vtkObjectBase* clientSideContourBase = contour->GetClientSideObject();
  vtkPyFRContourFilter* realContour =
    vtkPyFRContourFilter::SafeDownCast(clientSideContourBase);
  this->OutputData = vtkPyFRContourData::SafeDownCast(realContour->GetOutput());

  // Create a convertor to convert the pyfr contour data into polydata
  vtkSmartPointer<vtkSMSourceProxy> pyfrContourDataConverter;
  pyfrContourDataConverter.TakeReference(
    vtkSMSourceProxy::SafeDownCast(sessionProxyManager->
                                   NewProxy("filters",
                                            "PyFRContourDataConverter")));
  vtkSMInputProperty* pyfrContourDataConverterInputConnection =
    vtkSMInputProperty::SafeDownCast(pyfrContourDataConverter->
                                     GetProperty("Input"));

  producer->UpdateVTKObjects();
  pyfrContourDataConverterInputConnection->SetInputConnection(0, contour, 0);
  pyfrContourDataConverter->UpdatePropertyInformation();
  pyfrContourDataConverter->UpdateVTKObjects();
  controller->InitializeProxy(pyfrContourDataConverter);
  controller->RegisterPipelineProxy(pyfrContourDataConverter,
                                    "ConvertToPolyData");

  // Create a view
  vtkSmartPointer<vtkSMViewProxy> polydataViewer;
  polydataViewer.TakeReference(
    vtkSMViewProxy::SafeDownCast(sessionProxyManager->
                                 NewProxy("views","RenderView")));
  controller->InitializeProxy(polydataViewer);
  controller->RegisterViewProxy(polydataViewer);

  // Show the result.
  vtkSMProxy* representationBase = controller->
    Show(vtkSMSourceProxy::SafeDownCast(pyfrContourDataConverter), 0,
         vtkSMViewProxy::SafeDownCast(polydataViewer));

  this->Representation =
    vtkSMPVRepresentationProxy::SafeDownCast(representationBase);
  this->Representation->SetScalarColoring("pressure",0);
  this->Representation->SetScalarBarVisibility(polydataViewer,true);

  if (postFilterWrite)
    {
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
    }

  // stay in the loop while the simulation is paused
  while (true)
    {
    this->InsituLink->InsituUpdate(dataDescription->GetTime(),
                                   dataDescription->GetTimeStep());

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

    this->Representation->RescaleTransferFunctionToDataRange();

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
