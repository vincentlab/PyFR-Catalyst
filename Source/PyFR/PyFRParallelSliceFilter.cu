#include "PyFRParallelSliceFilter.h"

#include <vtkm/ImplicitFunctions.h>
#include <vtkm/cont/cuda/DeviceAdapterCuda.h>

#include "CrinkleClip.h"
#include "IsosurfaceHexahedra.h"
#include "PyFRData.h"
#include "PyFRContourData.h"

//----------------------------------------------------------------------------
PyFRParallelSliceFilter::PyFRParallelSliceFilter() : NPlanes(1), Spacing(1.)
{
  this->Origin[0] = this->Origin[1] = this->Origin[2] = 0.;
  this->Normal[0] = this->Normal[1] = 0.;
  this->Normal[2] = 1.;
}

//----------------------------------------------------------------------------
PyFRParallelSliceFilter::~PyFRParallelSliceFilter()
{
}

//----------------------------------------------------------------------------
void PyFRParallelSliceFilter::SetPlane(FPType origin_x,
                                       FPType origin_y,
                                       FPType origin_z,
                                       FPType normal_x,
                                       FPType normal_y,
                                       FPType normal_z)
{
  this->Origin[0] = origin_x;
  this->Origin[1] = origin_y;
  this->Origin[2] = origin_z;
  this->Normal[0] = normal_x;
  this->Normal[1] = normal_y;
  this->Normal[2] = normal_z;
}

//----------------------------------------------------------------------------
void PyFRParallelSliceFilter::operator()(PyFRData* input,
                                         PyFRContourData* output)
{
  typedef PyFRData::Vec3ArrayHandle CoordinateArrayHandle;
  typedef std::vector<vtkm::cont::ArrayHandle<vtkm::Vec<FPType,3> > >
    Vec3HandleVec;
  typedef std::vector<FPType> DataVec;
  typedef PyFRData::CellSet CellSet;

  const vtkm::cont::DataSet& dataSet = input->GetDataSet();

  CoordinateArrayHandle coords = dataSet.GetCoordinateSystem().GetData()
    .CastToArrayHandle(CoordinateArrayHandle::ValueType(),
                       CoordinateArrayHandle::StorageTag());

  vtkm::Plane func(vtkm::Vec<FPType,3>(this->Origin[0],
                                       this->Origin[1],
                                       this->Origin[2]),
                   vtkm::Vec<FPType,3>(this->Normal[0],
                                       this->Normal[1],
                                       this->Normal[2]));

  vtkm::ImplicitFunctionValue<vtkm::Plane> function(func);

  vtkm::cont::ArrayHandleTransform<FPType,CoordinateArrayHandle,
    vtkm::ImplicitFunctionValue<vtkm::Plane> > dataArray(coords,function);

  DataVec dataVec;
  Vec3HandleVec verticesVec;
  Vec3HandleVec normalsVec;
  output->SetNumberOfContours(this->NPlanes);
  for (unsigned i=0;i<output->GetNumberOfContours();i++)
    {
    dataVec.push_back(i*this->Spacing);
    verticesVec.push_back(output->GetContour(i).GetVertices());
    normalsVec.push_back(output->GetContour(i).GetNormals());
    }

  isosurfaceFilter.Run(dataVec,
                       dataSet.GetCellSet().CastTo(CellSet()),
                       dataSet.GetCoordinateSystem(),
                       dataArray,
                       verticesVec,
                       normalsVec);
}

//----------------------------------------------------------------------------
void PyFRParallelSliceFilter::MapFieldOntoSlices(int field,
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

  isosurfaceFilter.MapFieldOntoIsosurfaces<
    PyFRData::ScalarDataArrayHandle,
      PyFRContour::ScalarDataArrayHandle>(projectedArray,
                                          scalarDataHandleVec);
}
