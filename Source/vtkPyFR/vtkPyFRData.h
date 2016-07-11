#ifndef VTKPYFRDATA_H
#define VTKPYFRDATA_H

#include <vtkDataObject.h>

class PyFRData;

class VTK_EXPORT vtkPyFRData : public vtkDataObject
{
public:
  static vtkPyFRData* New();
  vtkTypeMacro(vtkPyFRData, vtkDataObject)
  virtual void PrintSelf(ostream& os, vtkIndent indent);

  void SetData(PyFRData* d) { data = d; }
  PyFRData* GetData() const { return data; }

protected:
  vtkPyFRData();
  virtual ~vtkPyFRData();

private:
  PyFRData* data;
};


vtkObject* New_vtkPyFRData();

#endif
