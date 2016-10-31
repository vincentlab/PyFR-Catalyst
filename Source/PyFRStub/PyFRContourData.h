#ifndef PYFRCONTOURDATA_H
#define PYFRCONTOURDATA_H

struct PyFRContourData
{
  void SetNumberOfContours(unsigned){};
  unsigned GetNumberOfContours() const { return 0; }
  unsigned GetContourSize(int) const { return 0; }
  void ComputeContourBounds(int,FPType*) const {}
  void ComputeBounds(FPType*) const {}
  void SetColorPalette(int,int,FPType,FPType) {}

  void SetColorPreset(int) {}
  void SetColorRange(FPType,FPType) {}
  void SetCustomColorTable(const uint8_t*, const float*, size_t) {}
};

namespace transfer
{
  static void coords(PyFRContourData*, int, unsigned int&) {}
  static void normals(PyFRContourData*, int, unsigned int&) {}
  static void colors(PyFRContourData*, int, unsigned int&) {}
}

#endif
