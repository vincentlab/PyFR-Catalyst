#ifndef PYFRDATA_H
#define PYFRDATA_H

#include <vector>
//State that the default backend for this code is CUDA
//not serial
//Disable treading support in our array handle
//needed for nvcc to stop complaining.
#define BOOST_SP_DISABLE_THREADS

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleCast.h>
#include <vtkm/cont/ArrayHandleCompositeVector.h>
#include <vtkm/cont/ArrayHandleImplicit.h>
#include <vtkm/cont/ArrayHandlePermutation.h>
#include <vtkm/cont/cuda/ArrayHandleCuda.h>
#include <vtkm/cont/cuda/DeviceAdapterCuda.h>
#include <vtkm/cont/DataSet.h>

#include "CatalystData.h"

struct StridedDataFunctor
{
  vtkm::Id NumberOfCells;
  vtkm::Id NVerticesPerCell;
  vtkm::Id NSolutionTypes;
  vtkm::Id SolutionType;
  vtkm::Id CellStride;
  vtkm::Id VertexStride;

  VTKM_EXEC_CONT_EXPORT
  vtkm::Id operator()(vtkm::Id index) const
  {
    vtkm::Id cell = index/NVerticesPerCell;
    vtkm::Id vertex = index%NVerticesPerCell;

    return cell + CellStride*SolutionType + VertexStride*vertex;
  }
};

/*
 * This class was adapted from the Isosurface class from Tom Fogal's
 * visualization plugin.
 */
class PyFRData
{
public:
  PyFRData();
  virtual ~PyFRData();

  typedef vtkm::cont::ArrayHandle<vtkm::Vec<FPType,3> > Vec3ArrayHandle;

  typedef vtkm::cont::ArrayHandleImplicit<vtkm::Id, StridedDataFunctor>
  DataIndexArrayHandle;

  typedef vtkm::cont::cuda::ArrayHandleCuda<FPType>::type RawDataArrayHandle;

  typedef vtkm::cont::ArrayHandlePermutation<DataIndexArrayHandle,
    RawDataArrayHandle> ScalarDataArrayHandle;

  typedef vtkm::cont::CellSetSingleType<> CellSet;

  void Init(void* field);

  vtkm::cont::DataSet& GetDataSet() { return dataSet; }
  const vtkm::cont::DataSet& GetDataSet() const { return dataSet; }

  void Update();

  static int FieldIndex(std::string name) { return PyFRData::fieldIndex[name]; }
  static std::string FieldName(int i) { return PyFRData::fieldName[i]; }

  std::vector<float> isovalues() const { return this->isovals; }

  // Should we output per-timestep metadata?  This is useful for configuring
  // subsequent runs, but impose extraneous global comm.
  bool PrintMetadata() const;

  /// Camera information as given by the user.
  ///@{
  float eye[3];
  float ref[3];
  float vup[3];
  ///@}

private:
  static std::map<int,std::string> fieldName;
  static std::map<std::string,int> fieldIndex;
  static bool mapsPopulated;
  static bool PopulateMaps();

  struct CatalystData* catalystData;
  vtkm::cont::DataSet dataSet;
  std::vector<float> isovals;
};

#endif
