#include "PyFRContourFilter.h"

#include "CrinkleClip.h"
#include "PyFRData.h"
#include "PyFRContourData.h"

//----------------------------------------------------------------------------
PyFRContourFilter::PyFRContourFilter() : ContourField(0)
{
}

//----------------------------------------------------------------------------
PyFRContourFilter::~PyFRContourFilter()
{
}

//----------------------------------------------------------------------------
void PyFRContourFilter::operator()(PyFRData* input,
                                   PyFRContourData* output)
{
  typedef std::vector<vtkm::cont::ArrayHandle<vtkm::Vec<FPType,3> > >
    Vec3HandleVec;
  typedef std::vector<FPType> DataVec;
  typedef vtkm::worklet::CrinkleClipTraits<typename PyFRData::CellSet>::CellSet
    CellSet;

  const vtkm::cont::DataSet& dataSet = input->GetDataSet();

  vtkm::cont::Field contourField =
    dataSet.GetField(PyFRData::FieldName(this->ContourField));
  PyFRData::ScalarDataArrayHandle contourArray = contourField.GetData()
    .CastToArrayHandle(PyFRData::ScalarDataArrayHandle::ValueType(),
                       PyFRData::ScalarDataArrayHandle::StorageTag());

  DataVec dataVec;
  Vec3HandleVec verticesVec;
  Vec3HandleVec normalsVec;
  output->SetNumberOfContours(this->ContourValues.size());
  for (unsigned i=0;i<output->GetNumberOfContours();i++)
    {
    dataVec.push_back(this->ContourValues[i]);
    verticesVec.push_back(output->GetContour(i).GetVertices());
    normalsVec.push_back(output->GetContour(i).GetNormals());
    }

  isosurfaceFilter.Run(dataVec,
                       dataSet.GetCellSet().CastTo(CellSet()),
                       dataSet.GetCoordinateSystem(),
                       contourArray,
                       verticesVec,
                       normalsVec);
}

//----------------------------------------------------------------------------
void PyFRContourFilter::MapFieldOntoIsosurfaces(int field,
                                                PyFRData* input,
                                                PyFRContourData* output)
{
  typedef std::vector<PyFRContour::ScalarDataArrayHandle> ScalarDataHandleVec;

  const vtkm::cont::DataSet& dataSet = input->GetDataSet();

  ScalarDataHandleVec scalarDataHandleVec;
  for (unsigned j=0;j<output->GetNumberOfContours();j++)
    {
    output->GetContour(j).SetScalarDataType(field);
    PyFRContour::ScalarDataArrayHandle scalars_out =
      output->GetContour(j).GetScalarData();

    scalarDataHandleVec.push_back(scalars_out);
    }

  vtkm::cont::Field projectedField =
    dataSet.GetField(PyFRData::FieldName(field));

  PyFRData::ScalarDataArrayHandle projectedArray = projectedField.GetData()
    .CastToArrayHandle(PyFRData::ScalarDataArrayHandle::ValueType(),
                       PyFRData::ScalarDataArrayHandle::StorageTag());

  isosurfaceFilter.MapFieldOntoIsosurfaces(projectedArray,
                                           scalarDataHandleVec);
}
