#ifndef VTKPYFRCRINKLECLIPFILTER_H
#define VTKPYFRCRINKLECLIPFILTER_H

#include <string>

#include "vtkPyFRDataAlgorithm.h"

class vtkImplicitFunction;
class vtkPlane;

class VTK_EXPORT vtkPyFRCrinkleClipFilter : public vtkPyFRDataAlgorithm
{
public:
  vtkTypeMacro(vtkPyFRCrinkleClipFilter,vtkPyFRDataAlgorithm)
  static vtkPyFRCrinkleClipFilter* New();
  virtual void PrintSelf(ostream& os, vtkIndent indent);

  void SetInputData(vtkDataObject*);
  void SetInputData(int,vtkDataObject*);
  int RequestData(vtkInformation*,vtkInformationVector**,vtkInformationVector*);
  int FillInputPortInformation(int,vtkInformation*);

  // Description:
  // Set/get plane normal. Plane is defined by point and normal.
  vtkSetVector3Macro(Normal,double);
  vtkGetVectorMacro(Normal,double,3);

  // Description:
  // Set/get point through which plane passes. Plane is defined by point
  // and normal.
  vtkSetVector3Macro(Origin,double);
  vtkGetVectorMacro(Origin,double,3);

protected:
  unsigned long LastExecuteTime;

  double Normal[3];
  double Origin[3];

  vtkPyFRCrinkleClipFilter();
  virtual ~vtkPyFRCrinkleClipFilter();

private:
  static int PyFRDataTypeRegistered;
  static int RegisterPyFRDataType();

  vtkPyFRCrinkleClipFilter(const vtkPyFRCrinkleClipFilter&); // Not implemented
  void operator=(const vtkPyFRCrinkleClipFilter&); // Not implemented
};
#endif
