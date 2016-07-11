#ifndef VTKPYFRDATACONVERTER_H
#define VTKPYFRDATACONVERTER_H

#include "vtkUnstructuredGridAlgorithm.h"

class VTK_EXPORT vtkPyFRDataConverter : public vtkUnstructuredGridAlgorithm
{
public:
  vtkTypeMacro(vtkPyFRDataConverter,vtkUnstructuredGridAlgorithm)
  static vtkPyFRDataConverter* New();
  virtual void PrintSelf(ostream& os, vtkIndent indent);

  int RequestData(vtkInformation*,vtkInformationVector**,vtkInformationVector*);

  int FillInputPortInformation(int,vtkInformation*);

protected:
  vtkPyFRDataConverter();
  virtual ~vtkPyFRDataConverter();

private:
  static int PyFRDataTypeRegistered;
  static int RegisterPyFRDataType();

  vtkPyFRDataConverter(const vtkPyFRDataConverter&);
  void operator=(const vtkPyFRDataConverter&);
};
#endif
