#ifndef PYFRCONTOURDATA_H
#define PYFRCONTOURDATA_H

#define BOOST_SP_DISABLE_THREADS

#include <vector>

#include "ColorTable.h"
#include "PyFRContour.h"

class PyFRContourData
{
public:
  PyFRContourData() : Table() {}
  virtual ~PyFRContourData() {}

  void SetNumberOfContours(unsigned);
  unsigned GetNumberOfContours()       const { return this->Contours.size(); }
  PyFRContour& GetContour(int i)             { return this->Contours[i]; }
  const PyFRContour& GetContour(int i) const { return this->Contours[i]; }
  unsigned GetContourSize(int) const;
  void ComputeContourBounds(int,FPType*) const;
  void ComputeBounds(FPType*) const;
  void SetColorPalette(int,FPType,FPType);

private:
  ColorTable Table;
  std::vector<PyFRContour> Contours;
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
