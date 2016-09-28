#ifndef VTKPYFRPIPELINE_H
#define VTKPYFRPIPELINE_H

#include <vtkCPPipeline.h>
#include <vtkSmartPointer.h>
#include <string>

class PyFRData;
class vtkCPDataDescription;
class vtkLiveInsituLink;
class vtkPyFRContourData;
class vtkPyFRMapper;
class vtkSMSourceProxy;
class vtkSMPVRepresentationProxy;
class vtkTextActor;
class vtkSMParaViewPipelineControllerWithRendering;

class vtkPyFRPipeline : public vtkCPPipeline
{
public:
  static vtkPyFRPipeline* New();
  vtkTypeMacro(vtkPyFRPipeline,vtkCPPipeline)
  virtual void PrintSelf(ostream& os, vtkIndent indent);

  virtual void Initialize(const char* hostName, int port, char* fileName,
                          vtkCPDataDescription* dataDescription);

  virtual int RequestDataDescription(vtkCPDataDescription* dataDescription);

  virtual int CoProcess(vtkCPDataDescription* dataDescription);

  vtkSmartPointer<vtkSMSourceProxy> GetContour() { return this->Contour; }
  vtkSmartPointer<vtkSMSourceProxy> GetSlice()   { return this->Slice;   }

protected:
  vtkPyFRPipeline();
  virtual ~vtkPyFRPipeline();

  const PyFRData* PyData(vtkCPDataDescription*) const;

private:
  vtkPyFRPipeline(const vtkPyFRPipeline&); // Not implemented
  void operator=(const vtkPyFRPipeline&); // Not implemented

  vtkLiveInsituLink* InsituLink;

  std::string FileName;

  vtkSmartPointer<vtkSMSourceProxy> Clip;
  vtkSmartPointer<vtkSMSourceProxy> Contour;
  vtkSmartPointer<vtkSMSourceProxy> Slice;

  vtkSmartPointer<vtkPyFRMapper> ContourMapper;
  vtkSmartPointer<vtkPyFRMapper> SliceMapper;

  vtkSmartPointer<vtkTextActor> Timestamp;

  vtkSmartPointer<vtkSMParaViewPipelineControllerWithRendering> controller;
};
#endif
