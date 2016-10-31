#include "PyFRData.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <iostream>
#include <iterator>
#include <limits>
#include <sstream>
#include <stdexcept>

#include <vtkm/CellShape.h>
#include <vtkm/CellTraits.h>
#include <vtkm/TopologyElementTag.h>
#include <vtkm/cont/CellSetSingleType.h>
#include <vtkm/cont/CoordinateSystem.h>
#include <vtkm/cont/DeviceAdapter.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/Field.h>
#include <vtkm/cont/cuda/DeviceAdapterCuda.h>

#include <vtkm/VectorAnalysis.h>
#include <vtkm/worklet/WorkletMapField.h>
#include <vtkm/worklet/DispatcherMapField.h>

#include "ArrayHandleExposed.h"

namespace vtkm {
namespace worklet {

struct ComputeMagnitude : public vtkm::worklet::WorkletMapField
{
  typedef void ControlSignature(
                                FieldIn<Scalar> u,
                                FieldIn<Scalar> v,
                                FieldIn<Scalar> w,
                                FieldOut<Scalar> mag
                                );
  typedef void ExecutionSignature(_1, _2, _3, _4);

  template<typename T>
  VTKM_EXEC_EXPORT void operator()(const T& u,
                                   const T& v,
                                   const T& w,
                                   T& mag) const
  {
    mag = vtkm::Magnitude(vtkm::make_Vec(u,v,w));
  }
};

}
}

//------------------------------------------------------------------------------
std::map<int,std::string> PyFRData::fieldName;
std::map<std::string,int> PyFRData::fieldIndex;
bool PyFRData::mapsPopulated = PyFRData::PopulateMaps();

//------------------------------------------------------------------------------
bool PyFRData::PopulateMaps()
{
  fieldName[0] = "density";
  fieldName[1] = "pressure";
  fieldName[2] = "velocity_u";
  fieldName[3] = "velocity_v";
  fieldName[4] = "velocity_w";
  fieldName[5] = "velocity_magnitude";
  fieldName[6] = "density_gradient_magnitude";
  fieldName[7] = "pressure_gradient_magnitude";
  fieldName[8] = "velocity_gradient_magnitude";
  fieldName[9] = "velocity_qcriterion";

  for (unsigned i=0;i<10;i++)
    fieldIndex[fieldName[i]] = i;

  return true;
}

//------------------------------------------------------------------------------
PyFRData::PyFRData() : catalystData(NULL)
{
  this->bg_color[0] = this->bg_color[1] = this->bg_color[2] = NAN;
  this->image_size[0] = this->image_size[1] = 400u;
}

//------------------------------------------------------------------------------
PyFRData::~PyFRData()
{
}

//------------------------------------------------------------------------------
void PyFRData::Init(void* data)
{
  this->catalystData = static_cast<struct CatalystData*>(data);

  // we only take data from the first stored cell type (i.e. hexahedra)
  MeshDataForCellType* meshData = &(this->catalystData->meshData[0]);
  SolutionDataForCellType* solutionData =
    &(this->catalystData->solutionData[0]);
  this->isovals.resize(this->catalystData->niso);
  std::copy(this->catalystData->isovalues,
            this->catalystData->isovalues+this->catalystData->niso,
            this->isovals.begin());
  std::copy(this->catalystData->eye, this->catalystData->eye+3, this->eye);
  std::copy(this->catalystData->ref, this->catalystData->ref+3, this->ref);
  std::copy(this->catalystData->vup, this->catalystData->vup+3, this->vup);

  typedef ::vtkm::cont::DeviceAdapterTagCuda CudaTag;

  Vec3ArrayHandle vertices;
    {
    const vtkm::Vec<FPType,3> *vecData =
      reinterpret_cast<const vtkm::Vec<FPType,3>*>(meshData->vertices);
    typedef vtkm::cont::internal::Storage<vtkm::Vec<FPType,3>,
                                       vtkm::cont::StorageTagBasic> Vec3Storage;
    Vec3ArrayHandle tmp =
      Vec3ArrayHandle(Vec3Storage(vecData,
                                  meshData->nCells*meshData->nVerticesPerCell));
    vtkm::cont::DeviceAdapterAlgorithm<CudaTag>().
      Copy(tmp, vertices);
    }

  vtkm::cont::ArrayHandle<vtkm::Id> connectivity;
    {
    vtkm::cont::ArrayHandle<int32_t> tmp =
      vtkm::cont::make_ArrayHandle(meshData->con,
                                   (meshData->nSubdividedCells*
                   vtkm::CellTraits<vtkm::CellShapeTagHexahedron>::NUM_POINTS));
    vtkm::cont::ArrayHandleCast<vtkm::Id,
      vtkm::cont::ArrayHandle<int32_t> > cast(tmp);
    vtkm::cont::DeviceAdapterAlgorithm<CudaTag>().
      Copy(cast, connectivity);
    }

  vtkm::cont::CellSetSingleType<> cset(vtkm::CellShapeTagHexahedron(),
                                       meshData->nCells*meshData->nVerticesPerCell,
                                       "cells");
  cset.Fill(connectivity);

  StridedDataFunctor stridedDataFunctor[5];
  for (unsigned i=0;i<5;i++)
    {
    stridedDataFunctor[i].NumberOfCells = meshData->nCells;
    stridedDataFunctor[i].NVerticesPerCell = meshData->nVerticesPerCell;
    stridedDataFunctor[i].NSolutionTypes = 5;
    stridedDataFunctor[i].SolutionType = i;
    stridedDataFunctor[i].CellStride = solutionData->lsdim;
    stridedDataFunctor[i].VertexStride = solutionData->ldim;
    }

  RawDataArrayHandle rawSolutionArray = vtkm::cont::cuda::make_ArrayHandle(
    static_cast<FPType*>(solutionData->solution),
    solutionData->ldim*meshData->nVerticesPerCell);

  DataIndexArrayHandle densityIndexArray(stridedDataFunctor[0],
                                   meshData->nCells*meshData->nVerticesPerCell);
  CatalystMappedDataArrayHandle densityArray(densityIndexArray, rawSolutionArray);

  DataIndexArrayHandle velocity_uIndexArray(stridedDataFunctor[1],
                                   meshData->nCells*meshData->nVerticesPerCell);
  CatalystMappedDataArrayHandle velocity_uArray(velocity_uIndexArray, rawSolutionArray);

  DataIndexArrayHandle velocity_vIndexArray(stridedDataFunctor[2],
                                   meshData->nCells*meshData->nVerticesPerCell);
  CatalystMappedDataArrayHandle velocity_vArray(velocity_vIndexArray, rawSolutionArray);

  DataIndexArrayHandle velocity_wIndexArray(stridedDataFunctor[3],
                                   meshData->nCells*meshData->nVerticesPerCell);
  CatalystMappedDataArrayHandle velocity_wArray(velocity_wIndexArray, rawSolutionArray);

  DataIndexArrayHandle pressureIndexArray(stridedDataFunctor[4],
                                   meshData->nCells*meshData->nVerticesPerCell);
  CatalystMappedDataArrayHandle pressureArray(pressureIndexArray, rawSolutionArray);

  //compute the magnitude of the velocity.
  ScalarDataArrayHandle velocity_mArray;
  vtkm::worklet::DispatcherMapField< vtkm::worklet::ComputeMagnitude, CudaTag > dispatcher;
  dispatcher.Invoke(velocity_uArray, velocity_vArray, velocity_wArray, velocity_mArray);

  enum ElemType { CONSTANT=0, LINEAR=1, QUADRATIC=2 };
  vtkm::cont::Field density("density",LINEAR,vtkm::cont::Field::ASSOC_POINTS,vtkm::cont::DynamicArrayHandle(densityArray));
  vtkm::cont::Field velocity_u("velocity_u",LINEAR,vtkm::cont::Field::ASSOC_POINTS,vtkm::cont::DynamicArrayHandle(velocity_uArray));
  vtkm::cont::Field velocity_v("velocity_v",LINEAR,vtkm::cont::Field::ASSOC_POINTS,vtkm::cont::DynamicArrayHandle(velocity_vArray));
  vtkm::cont::Field velocity_w("velocity_w",LINEAR,vtkm::cont::Field::ASSOC_POINTS,vtkm::cont::DynamicArrayHandle(velocity_wArray));
  vtkm::cont::Field velocity_m("velocity_magnitude",LINEAR,vtkm::cont::Field::ASSOC_POINTS,vtkm::cont::DynamicArrayHandle(velocity_mArray));
  vtkm::cont::Field pressure("pressure",LINEAR,vtkm::cont::Field::ASSOC_POINTS,vtkm::cont::DynamicArrayHandle(pressureArray));

  this->dataSet.AddCoordinateSystem(vtkm::cont::CoordinateSystem("coordinates",
                                                                 1,vertices));

  this->dataSet.AddField(density);
  this->dataSet.AddField(pressure);
  this->dataSet.AddField(velocity_u);
  this->dataSet.AddField(velocity_v);
  this->dataSet.AddField(velocity_w);
  this->dataSet.AddField(velocity_m);
  this->dataSet.AddCellSet(cset);
}

namespace {

PyFRData::CatalystMappedDataArrayHandle
make_CatalystHandle(const vtkm::cont::Field& field)
{
  return field.GetData().CastToArrayHandle(
    PyFRData::CatalystMappedDataArrayHandle::ValueType(),
    PyFRData::CatalystMappedDataArrayHandle::StorageTag());
}

PyFRData::ScalarDataArrayHandle
make_ScalarHandle(const vtkm::cont::Field& field)
{
  return field.GetData().CastToArrayHandle(
      PyFRData::ScalarDataArrayHandle::ValueType(),
      PyFRData::ScalarDataArrayHandle::StorageTag());
}

}

//------------------------------------------------------------------------------
void PyFRData::Update()
{
  typedef ::vtkm::cont::DeviceAdapterTagCuda CudaTag;

  //compute the magnitude of the velocity.
  vtkm::cont::Field veluField = this->dataSet.GetField("velocity_u");
  vtkm::cont::Field velvField = this->dataSet.GetField("velocity_v");
  vtkm::cont::Field velwField = this->dataSet.GetField("velocity_w");
  vtkm::cont::Field velmField = this->dataSet.GetField("velocity_magnitude");

  PyFRData::CatalystMappedDataArrayHandle velu = make_CatalystHandle(veluField);
  PyFRData::CatalystMappedDataArrayHandle velv = make_CatalystHandle(velvField);
  PyFRData::CatalystMappedDataArrayHandle velw = make_CatalystHandle(velwField);
  PyFRData::ScalarDataArrayHandle velm = make_ScalarHandle(velmField);

  vtkm::worklet::DispatcherMapField< vtkm::worklet::ComputeMagnitude, CudaTag > dispatcher;
  dispatcher.Invoke(velu, velv, velw, velm);

}

bool PyFRData::PrintMetadata() const {
  return this->catalystData->metadata;
}
