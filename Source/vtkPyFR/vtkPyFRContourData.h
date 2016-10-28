#ifndef VTKPYFRCONTOURDATA_H
#define VTKPYFRCONTOURDATA_H

#include <cinttypes>
#include <vtkDataObject.h>

class PyFRContourData;

class VTK_EXPORT vtkPyFRContourData : public vtkDataObject
{
public:
  static vtkPyFRContourData* New();
  vtkTypeMacro(vtkPyFRContourData, vtkDataObject)
  virtual void PrintSelf(ostream& os, vtkIndent indent);

  virtual void ShallowCopy(vtkPyFRContourData*);

  void SetData(PyFRContourData* d) { data = d; }
  PyFRContourData* GetData() const { return data; }

  int GetNumberOfContours() const;

  std::size_t GetSizeOfContour(int i) const;

  void ReleaseResources();

  void GetBounds(double*);
  const double* GetBounds();

  bool HasData() const;
  bool HasData(int i) const;

  double* GetColorRange() { return this->ColorRange; }
  void SetColorPalette(int,double*);
  void SetColorPreset(int);
  void SetColorRange(FPType, FPType);

protected:
  vtkPyFRContourData();
  virtual ~vtkPyFRContourData();

  double Bounds[6];
  unsigned int BoundsUpdateTime;

  int ColorPalette;
  double ColorRange[2];

private:
  PyFRContourData* data;
};

vtkObject* New_vtkPyFRContourData();

#endif
