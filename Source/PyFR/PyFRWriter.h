#ifndef PYFRWRITER_H
#define PYFRWRITER_H

#include <string>

#define BOOST_SP_DISABLE_THREADS

class PyFRData;
class PyFRContourData;

class PyFRWriter
{
public:
  PyFRWriter();
  virtual ~PyFRWriter();

  void operator()(PyFRData*) const;
  void operator()(PyFRContourData*) const;

  void SetFileName(std::string fileName) { FileName = fileName; }
  std::string GetFileName() const { return FileName; }

  void SetDataModeToAscii() { IsBinary = false; }
  void SetDataModeToBinary() { IsBinary = true; }

private:
  bool IsBinary;
  std::string FileName;
};
#endif
