#ifndef vtkPyFRParallelSliceFilter_h
#define vtkPyFRParallelSliceFilter_h

#include <string>
#include <vector>

#include "vtkPyFRContourDataAlgorithm.h"

class PyFRParallelSliceFilter;

class VTK_EXPORT vtkPyFRParallelSliceFilter : public vtkPyFRContourDataAlgorithm
{
public:
  vtkTypeMacro(vtkPyFRParallelSliceFilter,vtkPyFRContourDataAlgorithm)
  static vtkPyFRParallelSliceFilter* New();
  virtual void PrintSelf(ostream& os, vtkIndent indent);

  int RequestData(vtkInformation*,vtkInformationVector**,vtkInformationVector*);

  int FillInputPortInformation(int,vtkInformation*);

  vtkSetVector3Macro(Origin,double);
  vtkGetVectorMacro(Origin,double,3);

  vtkSetVector3Macro(Normal,double);
  vtkGetVectorMacro(Normal,double,3);

  vtkSetMacro(Spacing,double);
  vtkGetMacro(Spacing,double);

  vtkSetMacro(NumberOfPlanes,int);
  vtkGetMacro(NumberOfPlanes,int);

  vtkSetMacro(MappedField,int);
  vtkGetMacro(MappedField,int);

  vtkSetMacro(ColorPalette,int);
  vtkGetMacro(ColorPalette,int);

  vtkSetVector2Macro(ColorRange,double);
  vtkGetVectorMacro(ColorRange,double,2);

protected:
  vtkPyFRParallelSliceFilter();
  virtual ~vtkPyFRParallelSliceFilter();

  double Origin[3];
  double Normal[3];
  double Spacing;
  int NumberOfPlanes;
  int MappedField;
  int ColorPalette;
  double ColorRange[2];

  unsigned long LastExecuteTime;

  PyFRParallelSliceFilter* Filter;

private:
  static int PyFRDataTypesRegistered;
  static int RegisterPyFRDataTypes();

  vtkPyFRParallelSliceFilter(const vtkPyFRParallelSliceFilter&);
  void operator=(const vtkPyFRParallelSliceFilter&);
};
#endif
