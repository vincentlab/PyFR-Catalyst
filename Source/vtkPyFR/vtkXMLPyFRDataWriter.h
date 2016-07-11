#ifndef VTKXMLPYFRDATAWRITER_H
#define VTKXMLPYFRDATAWRITER_H

#include <string>

#include "vtkPyFRDataAlgorithm.h"

class VTK_EXPORT vtkXMLPyFRDataWriter : public vtkPyFRDataAlgorithm
{
public:
  vtkTypeMacro(vtkXMLPyFRDataWriter,vtkPyFRDataAlgorithm)
  static vtkXMLPyFRDataWriter* New();
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
  vtkXMLPyFRDataWriter();
  virtual ~vtkXMLPyFRDataWriter();

  void WriteData();

private:
  vtkXMLPyFRDataWriter(const vtkXMLPyFRDataWriter&); // Not implemented
  void operator=(const vtkXMLPyFRDataWriter&); // Not implemented

  bool IsBinary;
  std::string FileName;
};
#endif
