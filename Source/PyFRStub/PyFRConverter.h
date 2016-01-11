#ifndef PYFRCONVERTER_H
#define PYFRCONVERTER_H

class PyFRData;
class PyFRContourData;
class vtkPolyData;
class vtkUnstructuredGrid;

struct PyFRConverter
{
  void operator()(const PyFRData*,vtkUnstructuredGrid*) const {}
  void operator()(const PyFRContourData*,vtkPolyData*) const {}
};
#endif
