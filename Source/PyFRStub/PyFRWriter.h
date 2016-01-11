#ifndef PYFRWRITER_H
#define PYFRWRITER_H

class PyFRData;
class PyFRContourData;

struct PyFRWriter
{
  void operator()(PyFRData*) const {}
  void operator()(PyFRContourData*) const {}

  void SetFileName(std::string fileName) { FileName = fileName; }
  std::string GetFileName() const { return FileName; }

  void SetDataModeToAscii() { IsBinary = false; }
  void SetDataModeToBinary() { IsBinary = true; }

private:
  bool IsBinary;
  std::string FileName;
};
#endif
