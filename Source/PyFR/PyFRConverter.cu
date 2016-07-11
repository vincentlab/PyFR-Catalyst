#include <cmath>
#include <iomanip>
#include <string>
#include <sstream>

#include "PyFRConverter.h"

#include <vtkAppendPolyData.h>
#include <vtkCellArray.h>
#include <vtkCellType.h>
#include <vtkCommand.h>
#include <vtkCommunicator.h>
#include <vtkCompleteArrays.h>
#include <vtkCPDataDescription.h>
#include <vtkCPInputDataDescription.h>
#include <vtkDataArray.h>
#include <vtkDoubleArray.h>
#include "vtkErrorCode.h"
#include "vtkExecutive.h"
#include <vtkFloatArray.h>
#include <vtkHexahedron.h>
#include <vtkIdTypeArray.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkPointData.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>
#include <vtkSmartPointer.h>
#include <vtkUnstructuredGrid.h>

#include <vtkm/cont/ArrayHandleCast.h>
#include <vtkm/cont/DeviceAdapter.h>
#include <vtkm/cont/DeviceAdapterAlgorithm.h>
#include <vtkm/cont/DeviceAdapterSerial.h>
#include <vtkm/cont/DynamicArrayHandle.h>
#include <vtkm/cont/cuda/ArrayHandleCuda.h>
#include <vtkm/cont/cuda/DeviceAdapterCuda.h>

#include "ArrayHandleExposed.h"
#include "PyFRData.h"
#include "PyFRContour.h"
#include "PyFRContourData.h"

template <typename fptype>
struct ArrayChoice;

template <>
struct ArrayChoice<float>
{
  typedef vtkFloatArray type;
};

template <>
struct ArrayChoice<double>
{
  typedef vtkDoubleArray type;
};

//----------------------------------------------------------------------------
PyFRConverter::PyFRConverter()
{
}

//----------------------------------------------------------------------------
PyFRConverter::~PyFRConverter()
{
}

//----------------------------------------------------------------------------
void PyFRConverter::operator ()(const PyFRData* pyfrData,vtkUnstructuredGrid* grid) const
{
  const vtkm::cont::DataSet& dataSet = pyfrData->GetDataSet();

  namespace vtkmc = vtkm::cont;
  typedef vtkmc::ArrayHandleExposed<FPType> ScalarDataArrayHandleExposed;
  typedef vtkmc::ArrayHandleExposed<vtkm::Vec<FPType,3> >
    Vec3ArrayHandleExposed;
  typedef ::vtkm::cont::DeviceAdapterTagCuda CudaTag;

  Vec3ArrayHandleExposed vertices;
    {
    PyFRData::Vec3ArrayHandle tmp = dataSet.GetCoordinateSystem().GetData()
      .CastToArrayHandle(PyFRData::Vec3ArrayHandle::ValueType(),
                         PyFRData::Vec3ArrayHandle::StorageTag());
    vtkm::cont::DeviceAdapterAlgorithm<CudaTag>().
      Copy(tmp,vertices);
    }

  vtkSmartPointer<ArrayChoice<FPType>::type> pointData =
    vtkSmartPointer<ArrayChoice<FPType>::type>::New();

  vtkIdType nVerts = vertices.GetNumberOfValues();
  FPType* vertsArray = reinterpret_cast<FPType*>(vertices.Storage().StealArray());
  pointData->SetArray(vertsArray, nVerts*3,
                      0, // give VTK control of the data
                      0);// delete using "free"
  pointData->SetNumberOfComponents(3);

  vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
  points->SetData(pointData);

  vtkSmartPointer<ArrayChoice<FPType>::type> solutionData[5];
  for (unsigned i=0;i<5;i++)
    {
    vtkmc::Field solution = dataSet.GetField(PyFRData::FieldName(i));
    PyFRData::ScalarDataArrayHandle solutionArray = solution.GetData()
      .CastToArrayHandle(PyFRData::ScalarDataArrayHandle::ValueType(),
                         PyFRData::ScalarDataArrayHandle::StorageTag());
    ScalarDataArrayHandleExposed solutionArrayHost;
    vtkm::cont::DeviceAdapterAlgorithm<CudaTag>().
      Copy(solutionArray, solutionArrayHost);

    solutionData[i] = vtkSmartPointer<ArrayChoice<FPType>::type>::New();
    vtkIdType nSolution = solutionArrayHost.GetNumberOfValues();
    FPType* solutionArr = solutionArrayHost.Storage().StealArray();
    solutionData[i]->SetArray(solutionArr, nSolution,
                           0, // give VTK control of the data
                           0);// delete using "free"
    solutionData[i]->SetNumberOfComponents(1);
    solutionData[i]->SetName(PyFRData::FieldName(i).c_str());
    }

  PyFRData::CellSet cellSet = dataSet.GetCellSet().CastTo(PyFRData::CellSet());

    vtkm::cont::ArrayHandle<vtkm::Id> connectivity =
      cellSet.GetConnectivityArray(vtkm::TopologyElementTagPoint(),
                                   vtkm::TopologyElementTagCell());
    vtkm::cont::ArrayHandle<vtkm::Id>::PortalConstControl portal =
      connectivity.GetPortalConstControl();

  grid->Allocate(connectivity.GetNumberOfValues()/8);
  grid->SetPoints(points);
  for (unsigned i=0;i<5;i++)
    {
    grid->GetPointData()->AddArray(solutionData[i]);
    }
  vtkIdType counter = 0;
  while (counter < connectivity.GetNumberOfValues())
    {
    vtkSmartPointer<vtkHexahedron> hex = vtkSmartPointer<vtkHexahedron>::New();
    for (vtkIdType j=0;j<8;j++)
      {
      hex->GetPointIds()->SetId(j,portal.Get(counter++));
      }
    grid->InsertNextCell(hex->GetCellType(),hex->GetPointIds());
    }
}

//----------------------------------------------------------------------------
void PyFRConverter::operator ()(const PyFRContour& contour,vtkPolyData* polydata) const
{
  typedef ::vtkm::cont::DeviceAdapterTagCuda CudaTag;

  typedef vtkm::cont::ArrayHandleExposed<vtkm::Vec<FPType,3> > Vec3ArrayHandle;

  if (contour.GetVertices().GetNumberOfValues() == 0)
    {
    return;
    }

  PyFRContour::Vec3ArrayHandle verts_out;
  vtkm::cont::DeviceAdapterAlgorithm<CudaTag>().
    Copy(contour.GetVertices(),verts_out);

  vtkSmartPointer<ArrayChoice<FPType>::type> pointData =
    vtkSmartPointer<ArrayChoice<FPType>::type>::New();

  vtkIdType nVerts = verts_out.GetNumberOfValues();
  FPType* vertsArray = reinterpret_cast<FPType*>(verts_out.Storage().StealArray());
  pointData->SetArray(vertsArray, nVerts*3,
                      0, // give VTK control of the data
                      0);// delete using "free"
  pointData->SetNumberOfComponents(3);

  vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
  points->SetData(pointData);

  PyFRContour::Vec3ArrayHandle normals_out;
  vtkm::cont::DeviceAdapterAlgorithm<CudaTag>().
    Copy(contour.GetNormals(),normals_out);

  vtkSmartPointer<ArrayChoice<FPType>::type> normalsData =
    vtkSmartPointer<ArrayChoice<FPType>::type>::New();

  vtkIdType nNormals = normals_out.GetNumberOfValues();
  FPType* normalsArray = reinterpret_cast<FPType*>(normals_out.Storage().StealArray());
  normalsData->SetArray(normalsArray, nNormals*3,
                        0, // give VTK control of the data
                        0);// delete using "free"
  normalsData->SetNumberOfComponents(3);

  vtkSmartPointer<vtkCellArray> polys =
        vtkSmartPointer<vtkCellArray>::New();
  vtkIdType indices[3];
  for (vtkIdType i=0;i<points->GetNumberOfPoints();i+=3)
    {
    for (vtkIdType j=0;j<3;j++)
      {
      indices[j] = i+j;
      }
    polys->InsertNextCell(3,indices);
    }

  polydata->SetPoints(points);
  polydata->SetPolys(polys);
  polydata->GetPointData()->SetNormals(normalsData);

  PyFRContour::ScalarDataArrayHandle scalarsOut = contour.GetScalarData();
  vtkm::cont::ArrayHandleExposed<FPType> scalarsOutHost;
  vtkm::cont::DeviceAdapterAlgorithm<CudaTag>().
    Copy(scalarsOut,scalarsOutHost);

  vtkSmartPointer<ArrayChoice<FPType>::type> solutionData =
    vtkSmartPointer<ArrayChoice<FPType>::type>::New();
  vtkIdType nSolution = scalarsOutHost.GetNumberOfValues();
  FPType* solutionArray = scalarsOutHost.Storage().StealArray();
  solutionData->SetArray(solutionArray, nSolution,
                         0, // give VTK control of the data
                         0);// delete using "free"
  solutionData->SetNumberOfComponents(1);
  solutionData->SetName(PyFRData::FieldName(contour.GetScalarDataType()).c_str());

  polydata->GetPointData()->AddArray(solutionData);
}

//----------------------------------------------------------------------------
void PyFRConverter::operator ()(const PyFRContourData* pyfrContourData,vtkPolyData* outPolyData) const
{
  if (pyfrContourData->GetNumberOfContours() == 0)
    {
    return;
    }

  vtkSmartPointer<vtkAppendPolyData> appendFilter =
    vtkSmartPointer<vtkAppendPolyData>::New();
  appendFilter->SetOutput(outPolyData);

  std::vector<vtkPolyData*> polyData(pyfrContourData->GetNumberOfContours(),NULL);

  for (unsigned i=0;i<pyfrContourData->GetNumberOfContours();i++)
    {
    polyData[i] = vtkPolyData::New();
    this->operator()(pyfrContourData->GetContour(i),polyData[i]);
    appendFilter->AddInputData(polyData[i]);
    }

  appendFilter->Update();

  for (unsigned i=0;i<pyfrContourData->GetNumberOfContours();i++)
    {
    polyData[i]->Delete();
    }
}