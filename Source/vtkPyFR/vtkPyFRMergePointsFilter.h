#ifndef VTKPYFRMERGEPOINTSFILTER_H
#define VTKPYFRMERGEPOINTSFILTER_H

#include <string>

#include "vtkPyFRDataAlgorithm.h"

class vtkImplicitFunction;
class vtkPlane;

class VTK_EXPORT vtkPyFRMergePointsFilter : public vtkPyFRDataAlgorithm
{
public:
  vtkTypeMacro(vtkPyFRMergePointsFilter,vtkPyFRDataAlgorithm)
  static vtkPyFRMergePointsFilter* New();
  virtual void PrintSelf(ostream& os, vtkIndent indent);

  void SetInputData(vtkDataObject*);
  void SetInputData(int,vtkDataObject*);
  int RequestData(vtkInformation*,vtkInformationVector**,vtkInformationVector*);
  int FillInputPortInformation(int,vtkInformation*);

protected:
  unsigned long LastExecuteTime;

  vtkPyFRMergePointsFilter();
  virtual ~vtkPyFRMergePointsFilter();

private:
  static int PyFRDataTypeRegistered;
  static int RegisterPyFRDataType();

  vtkPyFRMergePointsFilter(const vtkPyFRMergePointsFilter&); // Not implemented
  void operator=(const vtkPyFRMergePointsFilter&); // Not implemented
};
#endif
