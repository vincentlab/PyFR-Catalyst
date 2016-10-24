#ifndef PYFRCONTOURDATA_H
#define PYFRCONTOURDATA_H

#define BOOST_SP_DISABLE_THREADS

#include <vector>

class PyFRContour;

class PyFRContourData
{
public:
  PyFRContourData();
  virtual ~PyFRContourData();

  void SetNumberOfContours(unsigned);
  unsigned GetNumberOfContours() const;
  PyFRContour& GetContour(int i);
  const PyFRContour& GetContour(int i) const;
  unsigned GetContourSize(int) const;
  void ComputeContourBounds(int,FPType*) const;
  void ComputeBounds(FPType*) const;
  void SetColorPalette(int,FPType,FPType);

  void SetColorPreset(int);
  void SetColorRange(FPType,FPType);
  void SetCustomColorTable(const uint8_t* rgba, const float* loc, size_t n);

private:
  class ContourDataImpl;
  ContourDataImpl* Impl;
  FPType crange[2];
};


//Methods that can be used to fill VBO's with a given contours data efficiently
//by using opengl interop, without having to move the data back to host
namespace transfer
{
  void coords(PyFRContourData* data, int index, unsigned int& glHandle);
  void normals(PyFRContourData* data, int index, unsigned int& glHandle);
  void colors(PyFRContourData* data, int index, unsigned int& glHandle);
}

#endif
