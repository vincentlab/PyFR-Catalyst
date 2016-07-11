#include <cmath>
#include <iomanip>
#include <string>
#include <sstream>

#include "PyFRWriter.h"

#include <vtkPolyData.h>
#include <vtkSmartPointer.h>
#include <vtkUnstructuredGrid.h>
#include <vtkXMLPolyDataWriter.h>
#include <vtkXMLUnstructuredGridWriter.h>

#include "PyFRConverter.h"
#include "PyFRData.h"
#include "PyFRContourData.h"

//----------------------------------------------------------------------------
PyFRWriter::PyFRWriter() : IsBinary(true),
                           FileName("output")
{
}

//----------------------------------------------------------------------------
PyFRWriter::~PyFRWriter()
{
}

//----------------------------------------------------------------------------
void PyFRWriter::operator ()(PyFRData* pyfrData) const
{
  PyFRConverter convert;
  vtkUnstructuredGrid* grid = vtkUnstructuredGrid::New();
  convert(pyfrData,grid);

  // Write the file
  vtkSmartPointer<vtkXMLUnstructuredGridWriter> writer =
    vtkSmartPointer<vtkXMLUnstructuredGridWriter>::New();
  std::stringstream s; s << FileName << ".vtu";
  writer->SetFileName(s.str().c_str());
  writer->SetInputData(grid);

  if (this->IsBinary)
    writer->SetDataModeToBinary();
  else
    writer->SetDataModeToAscii();

  writer->Write();
  grid->Delete();
}

//----------------------------------------------------------------------------
void PyFRWriter::operator ()(PyFRContourData* pyfrContourData) const
{
  PyFRConverter convert;
  vtkPolyData* polydata = vtkPolyData::New();
  convert(pyfrContourData,polydata);

  // Write the file
  vtkSmartPointer<vtkXMLPolyDataWriter> writer =
    vtkSmartPointer<vtkXMLPolyDataWriter>::New();
  std::stringstream s; s << FileName << ".vtp";
  writer->SetFileName(s.str().c_str());
  writer->SetInputData(polydata);

  if (this->IsBinary)
    writer->SetDataModeToBinary();
  else
    writer->SetDataModeToAscii();

  writer->Write();
  polydata->Delete();
}