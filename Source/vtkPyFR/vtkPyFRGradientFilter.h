#ifndef VTKPYFRGRADIENTFILTER_H
#define VTKPYFRGRADIENTFILTER_H

#include <string>

#include "vtkPyFRDataAlgorithm.h"

class vtkImplicitFunction;
class vtkPlane;

class VTK_EXPORT vtkPyFRGradientFilter : public vtkPyFRDataAlgorithm
{
public:
  vtkTypeMacro(vtkPyFRGradientFilter,vtkPyFRDataAlgorithm)
  static vtkPyFRGradientFilter* New();
  virtual void PrintSelf(ostream& os, vtkIndent indent);

  void SetInputData(vtkDataObject*);
  void SetInputData(int,vtkDataObject*);
  int RequestData(vtkInformation*,vtkInformationVector**,vtkInformationVector*);
  int FillInputPortInformation(int,vtkInformation*);

protected:
  vtkPyFRGradientFilter();
  virtual ~vtkPyFRGradientFilter();

private:
  static int PyFRDataTypeRegistered;
  static int RegisterPyFRDataType();

  vtkPyFRGradientFilter(const vtkPyFRGradientFilter&); // Not implemented
  void operator=(const vtkPyFRGradientFilter&); // Not implemented
};
#endif
