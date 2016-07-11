#ifndef VTKPYFRCONTOURDATACONVERTER_H
#define VTKPYFRCONTOURDATACONVERTER_H

#include "vtkPyFRContourDataAlgorithm.h"

class VTK_EXPORT vtkPyFRContourDataConverter :
  public vtkPyFRContourDataAlgorithm
{
public:
  vtkTypeMacro(vtkPyFRContourDataConverter,vtkPyFRContourDataAlgorithm)
  static vtkPyFRContourDataConverter* New();
  virtual void PrintSelf(ostream& os, vtkIndent indent);

  int RequestDataObject(
    vtkInformation *,
    vtkInformationVector **,
    vtkInformationVector *);

  int RequestData(vtkInformation*,vtkInformationVector**,vtkInformationVector*);

  int FillOutputPortInformation(int,vtkInformation*);

protected:
  vtkPyFRContourDataConverter();
  virtual ~vtkPyFRContourDataConverter();

private:
  static int PyFRDataTypeRegistered;
  static int RegisterPyFRDataType();

  vtkPyFRContourDataConverter(const vtkPyFRContourDataConverter&);
  void operator=(const vtkPyFRContourDataConverter&);
};
#endif
