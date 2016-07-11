#ifndef PYFRCONTOURDATA_H
#define PYFRCONTOURDATA_H

struct PyFRContourData
{
  unsigned GetNumberOfContours() const { return 0; }
  unsigned GetContourSize(int) const { return 0; }
  void ComputeContourBounds(int,FPType*) const {}
  void ComputeBounds(FPType*) const {}
  void SetColorPalette(int,FPType,FPType) {}
};

namespace transfer
{
  static void coords(PyFRContourData*, int, unsigned int&) {}
  static void normals(PyFRContourData*, int, unsigned int&) {}
  static void colors(PyFRContourData*, int, unsigned int&) {}
}

#endif
