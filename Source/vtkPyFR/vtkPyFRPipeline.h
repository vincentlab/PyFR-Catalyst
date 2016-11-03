#ifndef VTKPYFRPIPELINE_H
#define VTKPYFRPIPELINE_H

#include <cinttypes>
#include <string>
#include <vtkCPPipeline.h>
#include <vtkSmartPointer.h>

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

  //pipelineMode 1=contour
  //pipelineMode 2=slice
  virtual void Initialize(const char* hostName, int port, char* fileName,
                          vtkCPDataDescription* dataDescription);

  virtual int RequestDataDescription(vtkCPDataDescription* dataDescription);

  virtual void SetResolution(uint32_t w, uint32_t h);
  virtual void SetSpecularLighting(float coefficient, float power);
  virtual int CoProcess(vtkCPDataDescription* dataDescription);

  virtual void SetColorTable(const uint8_t* rgba, const float* loc, size_t n, int pipeline);
  virtual void SetColorRange(FPType, FPType, int pipeline);
  virtual void SetFieldToColorBy(int, int);

  virtual void SetFieldToContourBy(int);
  virtual void SetSlicePlanes(float origin[3], float normal[3],
                              int number, double spacing);

  virtual void SetClipPlanes(float origin1[3], float normal1[3],
                             float origin2[3], float normal2[3]);

  vtkSmartPointer<vtkSMSourceProxy> GetContour() { return this->Contour; }
  vtkSmartPointer<vtkSMSourceProxy> GetSlice()   { return this->Slice;   }

protected:
  vtkPyFRPipeline();
  virtual ~vtkPyFRPipeline();

  const PyFRData* PyData(vtkCPDataDescription*) const;

  void InitPipeline1(vtkSmartPointer<vtkSMSourceProxy> input,
                     vtkCPDataDescription* dataDescription);
  void InitPipeline2(vtkSmartPointer<vtkSMSourceProxy> input,
                     vtkCPDataDescription* dataDescription);
  void DumpToFile(vtkCPDataDescription* dataDescription);

private:
  vtkPyFRPipeline(const vtkPyFRPipeline&); // Not implemented
  void operator=(const vtkPyFRPipeline&); // Not implemented

  vtkLiveInsituLink* InsituLink;

  std::string FileName;

  vtkSmartPointer<vtkSMSourceProxy> Clip1;
  vtkSmartPointer<vtkSMSourceProxy> Clip2;
  vtkSmartPointer<vtkSMSourceProxy> Contour;
  vtkSmartPointer<vtkSMSourceProxy> Slice;

  vtkSmartPointer<vtkPyFRMapper> ActiveMapper1, ActiveMapper2;

  vtkSmartPointer<vtkTextActor> Timestamp;

  vtkSmartPointer<vtkSMParaViewPipelineControllerWithRendering> controller;
};
#endif
