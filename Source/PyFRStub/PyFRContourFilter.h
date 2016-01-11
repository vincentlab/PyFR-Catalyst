#ifndef PYFRCONTOURFILTER_H
#define PYFRCONTOURFILTER_H

class PyFRData;
class PyFRContourData;

struct PyFRContourFilter
{
  void operator ()(PyFRData*,PyFRContourData*) const {}
  void MapFieldOntoIsosurfaces(int,PyFRData*,PyFRContourData*) {}

  void AddContourValue(FPType) {}
  void ClearContourValues() {}

  void SetContourField(int) {}
}
;
#endif
