#ifndef VTKXMLPYFRCONTOURDATAWRITER_H
#define VTKXMLPYFRCONTOURDATAWRITER_H

#include <string>

#include "vtkPyFRContourDataAlgorithm.h"

class VTK_EXPORT vtkXMLPyFRContourDataWriter : public vtkPyFRContourDataAlgorithm
{
public:
  vtkTypeMacro(vtkXMLPyFRContourDataWriter,vtkPyFRContourDataAlgorithm)
  static vtkXMLPyFRContourDataWriter* New();
  virtual void PrintSelf(ostream& os, vtkIndent indent);

  void SetFileName(std::string fileName) { FileName = fileName; }
  std::string GetFileName() const { return FileName; }
  void SetInputData(vtkDataObject *);
  void SetInputData(int, vtkDataObject*);

  void SetDataModeToAscii() { IsBinary = false; }
  void SetDataModeToBinary() { IsBinary = true; }

  int RequestData(vtkInformation*,vtkInformationVector**,vtkInformationVector*);

  int Write();

protected:
  vtkXMLPyFRContourDataWriter();
  virtual ~vtkXMLPyFRContourDataWriter();

  void WriteData();

private:
  vtkXMLPyFRContourDataWriter(const vtkXMLPyFRContourDataWriter&); // Not implemented
  void operator=(const vtkXMLPyFRContourDataWriter&); // Not implemented

  bool IsBinary;
  std::string FileName;
};
#endif
