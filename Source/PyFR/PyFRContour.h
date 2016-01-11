#ifndef PYFRCONTOUR_H
#define PYFRCONTOUR_H

#define BOOST_SP_DISABLE_THREADS

#include <string>

#include "ArrayHandleExposed.h"
#include "ColorTable.h"

#include <vtkm/cont/ArrayHandleTransform.h>

class PyFRContour
{
public:
  typedef vtkm::cont::ArrayHandleExposed<vtkm::Vec<FPType,3> > Vec3ArrayHandle;
  typedef vtkm::cont::ArrayHandleExposed<Color> ColorArrayHandle;
  typedef vtkm::cont::ArrayHandleTransform<FPType,
                                           ColorArrayHandle,
                                           ColorTable,
                                           ColorTable> ScalarDataArrayHandle;

  PyFRContour(const ColorTable& table) : Vertices(),
                                         Normals(),
                                         ColorData(),
                                         ScalarData(this->ColorData,
                                                    table,
                                                    table),
                                         ScalarDataType(-1)
  {
  }

  ~PyFRContour() {}

  Vec3ArrayHandle GetVertices()         const { return this->Vertices; }
  Vec3ArrayHandle GetNormals()          const { return this->Normals; }
  ScalarDataArrayHandle GetScalarData() const { return this->ScalarData; }
  ColorArrayHandle GetColorData()       const { return this->ColorData; }
  int GetScalarDataType()               const { return this->ScalarDataType; }

  void ChangeColorTable(const ColorTable& table)
  {
    this->ScalarData = ScalarDataArrayHandle(this->ColorData,table,table);
  }

  void SetScalarDataType(int i) { this->ScalarDataType = i; }

private:
  Vec3ArrayHandle Vertices;
  Vec3ArrayHandle Normals;
  ColorArrayHandle ColorData;
  ScalarDataArrayHandle ScalarData;
  int ScalarDataType;
};

#endif
