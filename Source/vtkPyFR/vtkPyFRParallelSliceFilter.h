#ifndef vtkPyFRParallelSliceFilter_h
#define vtkPyFRParallelSliceFilter_h

#include <string>
#include <vector>

#include "vtkPyFRContourDataAlgorithm.h"

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

  void SetMappedField(int i);
  vtkGetMacro(MappedField,int);

  void SetColorPalette(int);
  vtkGetMacro(ColorPalette,int);

  void SetColorRange(double, double);
  vtkGetVectorMacro(ColorRange,double,2);

protected:
  vtkPyFRParallelSliceFilter();
  virtual ~vtkPyFRParallelSliceFilter();

  double Origin[3];
  double Normal[3];
  double Spacing;
  int NumberOfPlanes;
  int MappedField;

  bool ColorPaletteNeedsSyncing;
  int ColorPalette;
  double ColorRange[2];

private:
  static int PyFRDataTypesRegistered;
  static int RegisterPyFRDataTypes();

  vtkPyFRParallelSliceFilter(const vtkPyFRParallelSliceFilter&);
  void operator=(const vtkPyFRParallelSliceFilter&);
};
#endif
