//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 Sandia Corporation.
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

#ifndef vtk_m_worklet_IsosurfaceHexahedra_h
#define vtk_m_worklet_IsosurfaceHexahedra_h

#include <cassert>
#include <stdlib.h>
#include <stdio.h>

#include <boost/static_assert.hpp>

#include <vtkm/cont/DeviceAdapter.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleCounting.h>
#include <vtkm/cont/ArrayHandlePermutation.h>
#include <vtkm/cont/CellSetExplicit.h>
#include <vtkm/cont/DynamicArrayHandle.h>
#include <vtkm/worklet/DispatcherMapField.h>
#include <vtkm/worklet/WorkletMapField.h>
#include <vtkm/Pair.h>

#include <vtkm/cont/CellSetPermutation.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/Field.h>
#include <vtkm/worklet/DispatcherMapTopology.h>
#include <vtkm/worklet/WorkletMapTopology.h>
#include <vtkm/VectorAnalysis.h>

#include <vtkm/worklet/MarchingCubesDataTables.h>

#include <vtkm/exec/Assert.h>

namespace vtkm {
namespace worklet {
namespace internal {

/// \brief Compute the isosurface for a uniform grid data set
template <typename FieldType, typename DeviceAdapter,
  vtkm::IdComponent NumberOfIsovalues>
class IsosurfaceFilterHexahedra
{
  BOOST_STATIC_ASSERT(NumberOfIsovalues > 0);
protected:
  typedef vtkm::IdComponent IsovalueCount;

  typedef vtkm::Vec<vtkm::Id,NumberOfIsovalues> IdVec;
  typedef vtkm::cont::ArrayHandle<IdVec> IdVecHandle;
  typedef vtkm::cont::ArrayHandle<vtkm::Id> IdHandle;
  typedef std::vector<IdHandle> IdHandleVec;

  typedef std::vector<FieldType> FieldVec;
  typedef vtkm::cont::ArrayHandle<FieldType> FieldHandle;
  typedef std::vector<FieldHandle> FieldHandleVec;

public:
  class ClassifyCell : public vtkm::worklet::WorkletMapPointToCell
  {
  public:
    typedef vtkm::ListTagBase<IdVec> IdVecType;

    typedef void ControlSignature(FieldInFrom<Scalar> scalars,
                                  TopologyIn topology,
                                  FieldOut<IdVecType> numVertices);
    typedef void ExecutionSignature(_1, _3);
    typedef _2 InputDomain;

    typedef vtkm::cont::ArrayHandle<vtkm::Id> IdArrayHandle;
    typedef typename IdArrayHandle::ExecutionTypes<DeviceAdapter>::PortalConst IdPortalType;
    const IdPortalType VertexTable;
    vtkm::Vec<FieldType,NumberOfIsovalues> Isovalues;

    VTKM_CONT_EXPORT
    ClassifyCell(const IdPortalType vertexTable,
                 const FieldVec& isovalues) :
      VertexTable(vertexTable)
    {
      for (unsigned i=0;i<NumberOfIsovalues;i++)
        this->Isovalues[i] = isovalues[i];
    }

    template<typename ScalarsVecType>
    VTKM_EXEC_EXPORT
    void operator()(const ScalarsVecType &scalars,
                    IdVec& numVertices) const
    {
      const vtkm::Id mask[] = { 1, 2, 4, 8, 16, 32, 64, 128 };

#pragma unroll
      for (IsovalueCount iso=0;iso<NumberOfIsovalues;iso++)
        {
        vtkm::Id caseId = 0;
        for (vtkm::IdComponent i = 0; i < 8; ++i)
          caseId += (static_cast<FieldType>(scalars[i]) > this->Isovalues[iso])*mask[i];
        numVertices[iso] = this->VertexTable.Get(caseId) / 3;
        }
    }
  };

  /// \brief Compute isosurface vertices and scalars
  class IsoSurfaceGenerate : public vtkm::worklet::WorkletMapPointToCell
  {
  public:
    typedef void ControlSignature(FieldInFrom<Scalar> scalars,
                                  FieldInFrom<Vec3> coordinates,
                                  FieldInTo<IdType> inputLowerBounds,
                                  TopologyIn topology);
    typedef void ExecutionSignature(WorkIndex, _1, _2, _3, FromIndices);
    typedef _4 InputDomain;

    const FieldType Isovalue;

    typedef typename vtkm::cont::ArrayHandle<FieldType>::template ExecutionTypes<DeviceAdapter>::Portal ScalarPortalType;
    ScalarPortalType InterpolationWeight;

    typedef typename vtkm::cont::ArrayHandle<vtkm::Id> IdArrayHandle;
    typedef typename IdArrayHandle::ExecutionTypes<DeviceAdapter>::PortalConst IdPortalConstType;
    IdPortalConstType TriTable;
    typedef typename IdArrayHandle::ExecutionTypes<DeviceAdapter>::Portal IdPortalType;
    IdPortalType InterpolationLowId;
    IdPortalType InterpolationHighId;

    typedef typename vtkm::cont::ArrayHandle<vtkm::Vec<FieldType, 3> >::template ExecutionTypes<DeviceAdapter>::Portal VectorPortalType;
    VectorPortalType Vertices;
    // VectorPortalType Normals;

    template<typename V>
    VTKM_CONT_EXPORT
    IsoSurfaceGenerate(const FieldType ivalue,
                       IdPortalConstType triTablePortal,
                       ScalarPortalType interpolationWeight,
                       IdPortalType interpolationLowId,
                       IdPortalType interpolationHighId,
                       const V &vertices) :
      Isovalue(ivalue),
      InterpolationWeight(interpolationWeight),
      TriTable(triTablePortal),
      InterpolationLowId(interpolationLowId),
      InterpolationHighId(interpolationHighId),
      Vertices(vertices)//,
      // Normals(normals)
    {
    }

    template<typename ScalarsVecType,typename VectorsVecType,typename IdVecType>
    VTKM_EXEC_EXPORT
    void operator()(const vtkm::Id outputCellId,
                    const ScalarsVecType &scalars,
                    const VectorsVecType &pointCoords,
                    const vtkm::Id inputLowerBounds,
                    const IdVecType &pointIds) const
    {
      // Get data for this cell
      const int verticesForEdge[] = { 0, 1, 1, 2, 3, 2, 0, 3,
                                      4, 5, 5, 6, 7, 6, 4, 7,
                                      0, 4, 1, 5, 2, 6, 3, 7 };

      // Compute the Marching Cubes case number for this cell
      unsigned int cubeindex = 0;
      const vtkm::Id mask[] = { 1, 2, 4, 8, 16, 32, 64, 128 };
#pragma unroll
      for (vtkm::IdComponent i = 0; i < 8; ++i)
        cubeindex += (static_cast<FieldType>(scalars[i]) > this->Isovalue)*mask[i];

      // Interpolate for vertex positions and associated scalar values
      const vtkm::Id inputIteration = (outputCellId - inputLowerBounds);
      const vtkm::Id outputVertId = outputCellId * 3;
      const vtkm::Id cellOffset = (static_cast<vtkm::Id>(cubeindex*16) +
                                   (inputIteration * 3));

      for (vtkm::IdComponent v = 0; v < 3; v++)
      {
        const vtkm::Id edge = this->TriTable.Get(cellOffset + v);
        const int v0   = verticesForEdge[2*edge];
        const int v1   = verticesForEdge[2*edge + 1];
        const FieldType t  = (this->Isovalue - scalars[v0]) / (scalars[v1] - scalars[v0]);
        this->Vertices.Set(outputVertId + v,
                           vtkm::Lerp(pointCoords[v0], pointCoords[v1], t));
        this->InterpolationWeight.Set(outputVertId + v, t);
        this->InterpolationLowId.Set(outputVertId + v, pointIds[v0]);
        this->InterpolationHighId.Set(outputVertId + v, pointIds[v1]);
      }

      //disabling normal calculation as it is not needed
      // vtkm::Vec<FieldType, 3> vertex0 = this->Vertices.Get(outputVertId + 0);
      // vtkm::Vec<FieldType, 3> vertex1 = this->Vertices.Get(outputVertId + 1);
      // vtkm::Vec<FieldType, 3> vertex2 = this->Vertices.Get(outputVertId + 2);

      // vtkm::Vec<FieldType, 3> curNorm = vtkm::Cross(vertex1-vertex0,
      //                                               vertex2-vertex0);
      // vtkm::Normalize(curNorm);
      // this->Normals.Set(outputVertId + 0, curNorm);
      // this->Normals.Set(outputVertId + 1, curNorm);
      // this->Normals.Set(outputVertId + 2, curNorm);
    }
  };

  template <typename Field>
  class ApplyToField : public vtkm::worklet::WorkletMapField
  {
  public:
    typedef void ControlSignature(FieldIn<Scalar> interpolationLow,
                                  FieldIn<Scalar> interpolationHigh,
                                  FieldIn<Scalar> interpolationWeight,
                                  FieldOut<Scalar> interpolatedOutput);
    typedef void ExecutionSignature(_1, _2, _3, _4);
    typedef _1 InputDomain;

    VTKM_CONT_EXPORT
    ApplyToField()
    {
    }

    VTKM_EXEC_EXPORT
    void operator()(const Field &low,
                    const Field &high,
                    const FieldType &weight,
                    Field &result) const
    {
      result = vtkm::Lerp(low, high, weight);
    }
  };

  class SingleId
  {
  public:
    typedef typename IdVecHandle::ValueType Vec;
    typedef typename Vec::ComponentType T;
    typedef vtkm::cont::ArrayHandleTransform<T,IdVecHandle,SingleId> Array;

    vtkm::Id operator()(Vec vec) const
    {
      return vec[this->Isovalue];
    }

    void SetIsovalue(IsovalueCount iso) { this->Isovalue = iso; }
  private:
    IsovalueCount Isovalue;
  };

  public:
  IsosurfaceFilterHexahedra() {}

  template<class CellSetType,typename StorageTag,typename CoordinateType>
  static void Run(const FieldVec& isovalues,
                  const CellSetType& cellSet,
                  const vtkm::cont::CoordinateSystem& coordinateSystem,
                  const vtkm::cont::ArrayHandle<FieldType,StorageTag>& isoField,
                  std::vector<vtkm::cont::ArrayHandle<vtkm::Vec<CoordinateType,3> > >& vertices,
                  std::vector<vtkm::cont::ArrayHandle<vtkm::Vec<CoordinateType,3> > >& normals,
                  FieldHandleVec& interpolationWeights,
                  IdHandleVec& interpolationLowIds,
                  IdHandleVec& interpolationHighIds)
  {
    typedef typename vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapter> DeviceAlgorithms;

    // Set up the Marching Cubes case tables
    IdHandle vertexTableArray =
    vtkm::cont::make_ArrayHandle(vtkm::worklet::internal::numVerticesTable,256);

    // Call the ClassifyCell functor to compute the Marching Cubes case numbers
    // for each cell, and the number of vertices to be generated
    ClassifyCell classifyCell(vertexTableArray.PrepareForInput(DeviceAdapter()),
                              isovalues);

    typedef typename vtkm::worklet::DispatcherMapTopology<
                                      ClassifyCell,
                                      DeviceAdapter> ClassifyCellDispatcher;
    ClassifyCellDispatcher classifyCellDispatcher(classifyCell);

    IdVecHandle numOutputTrisPerCell;
    classifyCellDispatcher.Invoke(isoField,
                                  cellSet,
                                  numOutputTrisPerCell);

    IdVecHandle local;
    DeviceAlgorithms::Copy(numOutputTrisPerCell,local);

    typename IdVecHandle::PortalConstControl portal = local.GetPortalConstControl();
    //now manually compute the sum
    IdVec sum = portal.Get(0);
    for( std::size_t i = 1; i < local.GetNumberOfValues(); ++i)
      {
      for (unsigned j=0;j<NumberOfIsovalues;j++)
        sum[j] += portal.Get(i)[j];
      }

    // Compute the number of valid input cells and those ids
    IdVec NumOutputCells =
      DeviceAlgorithms::ScanInclusive(numOutputTrisPerCell,
                                      numOutputTrisPerCell);

    vtkm::cont::ArrayHandle<vtkm::Id> triangleTableArray =
      vtkm::cont::make_ArrayHandle(vtkm::worklet::internal::triTable,256*16);

    SingleId singleId;
    for (IsovalueCount iso=0;iso<NumberOfIsovalues;iso++)
      {
      if (NumOutputCells[iso] <= 0)
        {
        interpolationWeights[iso].Shrink(0);
        interpolationLowIds[iso].Shrink(0);
        interpolationHighIds[iso].Shrink(0);
        vertices[iso].Shrink(0);
        // normals[iso].Shrink(0);
        continue;
        }

      singleId.SetIsovalue(iso);
      typename SingleId::Array numOutputTrisPerCell_single(numOutputTrisPerCell,
                                                           singleId);

      IdHandle validCellIndicesArray;
      IdHandle inputCellIterationNumber;
      vtkm::cont::ArrayHandleCounting<vtkm::Id> validCellCountImplicitArray(0, 1, NumOutputCells[iso]);

      DeviceAlgorithms::UpperBounds(numOutputTrisPerCell_single,
                                    validCellCountImplicitArray,
                                    validCellIndicesArray);

      // Compute for each output triangle what iteration of the input cell
      // generates it
      DeviceAlgorithms::LowerBounds(validCellIndicesArray,
                                    validCellIndicesArray,
                                    inputCellIterationNumber);

      // Generate a single triangle per cell
      const vtkm::Id numTotalVertices = NumOutputCells[iso] * 3;

      IsoSurfaceGenerate isosurface(
        isovalues[iso],
        triangleTableArray.PrepareForInput(DeviceAdapter()),
        interpolationWeights[iso].PrepareForOutput(numTotalVertices,
                                                   DeviceAdapter()),
        interpolationLowIds[iso].PrepareForOutput(numTotalVertices,
                                                  DeviceAdapter()),
        interpolationHighIds[iso].PrepareForOutput(numTotalVertices,
                                                   DeviceAdapter()),
        vertices[iso].PrepareForOutput(numTotalVertices, DeviceAdapter())// ,
        // normals[iso].PrepareForOutput(numTotalVertices, DeviceAdapter())
        );


      vtkm::cont::CellSetPermutation<vtkm::cont::ArrayHandle<vtkm::Id>,
        CellSetType> cellPermutation(validCellIndicesArray,
                                     cellSet);

      typedef typename vtkm::worklet::DispatcherMapTopology< IsoSurfaceGenerate,
        DeviceAdapter> IsoSurfaceDispatcher;
      IsoSurfaceDispatcher isosurfaceDispatcher(isosurface);
      isosurfaceDispatcher.Invoke(isoField,
                                  coordinateSystem.GetData(),
                                  inputCellIterationNumber,
                                  cellPermutation);

      }
  }

  template<typename ArrayHandleIn, typename ArrayHandleOut>
  static void MapFieldOntoIsosurfaces(const ArrayHandleIn& fieldIn,
                                      const FieldHandleVec& interpolationWeights,
                                      const IdHandleVec& interpolationLowIds,
                                      const IdHandleVec& interpolationHighIds,
                                      std::vector<ArrayHandleOut>& fieldOut)
  {
    assert(fieldOut.size() == NumberOfIsovalues);
    for (IsovalueCount iso = 0; iso < NumberOfIsovalues; iso++)
      {
      if (interpolationWeights[iso].GetNumberOfValues() == 0)
        {
        fieldOut[iso].Shrink(0);
        continue;
        }

      typedef vtkm::cont::ArrayHandlePermutation<IdHandle,
        ArrayHandleIn> FieldPermutationHandleType;

      FieldPermutationHandleType low(interpolationLowIds[iso],fieldIn);
      FieldPermutationHandleType high(interpolationHighIds[iso],fieldIn);

      ApplyToField<typename ArrayHandleIn::ValueType> applyToField;

      typedef typename vtkm::worklet::DispatcherMapField<
        ApplyToField<typename ArrayHandleIn::ValueType>,
        DeviceAdapter> ApplyToFieldDispatcher;
      ApplyToFieldDispatcher applyToFieldDispatcher(applyToField);

      applyToFieldDispatcher.Invoke(low,
                                    high,
                                    interpolationWeights[iso],
                                    fieldOut[iso]);
      }
  }
};

}
}
} // namespace vtkm::worklet::internal

namespace vtkm {
namespace worklet {

template <typename FieldType, typename DeviceAdapter,
  vtkm::IdComponent MaxNumberOfIsovalues=6>
class IsosurfaceFilterHexahedra
{
public:
  typedef vtkm::IdComponent IsovalueCount;
  typedef std::vector<FieldType> FieldVec;

  typedef std::vector<vtkm::cont::ArrayHandle<FieldType> > FieldHandleVec;

  typedef vtkm::cont::ArrayHandle<vtkm::Id> IdHandle;
  typedef std::vector<IdHandle> IdHandleVec;

private:
  template<typename StorageTag,typename CoordinateType>
  class RunOnCellTypeFunctor
  {
  public:
    typedef IsosurfaceFilterHexahedra<FieldType,DeviceAdapter,
    MaxNumberOfIsovalues> IsosurfaceFilter;

    typedef vtkm::cont::ArrayHandle<FieldType,StorageTag> FieldHandle;

    typedef vtkm::Vec<CoordinateType,3> Vec3;
    typedef vtkm::cont::ArrayHandle<Vec3> Vec3Handle;
    typedef std::vector<Vec3Handle> Vec3HandleVec;

    RunOnCellTypeFunctor(IsosurfaceFilter* filter,
                         const FieldVec& isovalues,
                         const vtkm::cont::CoordinateSystem& coords,
                         const FieldHandle& isoField,
                         Vec3HandleVec& vertices,
                         Vec3HandleVec& normals) : Filter(filter),
                                                   Isovalues(isovalues),
                                                   CoordinateSystem(coords),
                                                   IsoField(isoField),
                                                   Vertices(vertices),
                                                   Normals(normals) {}

    template<typename CellSetType>
    void operator() (const CellSetType& cellSet) const
    {
      return this->Filter->Run(this->Isovalues,
                               cellSet,
                               this->CoordinateSystem,
                               this->IsoField,
                               Vertices,
                               Normals);
    }

  private:
    IsosurfaceFilter* Filter;
    const FieldVec& Isovalues;
    const vtkm::cont::CoordinateSystem& CoordinateSystem;
    const FieldHandle& IsoField;
    Vec3HandleVec& Vertices;
    Vec3HandleVec& Normals;
  };

  template<class CellSetType,typename StorageTag,typename CoordinateType,
    IsovalueCount NumberOfIsovalues=1>
  class RunOverIsocontourSetFunctor
  {
  public:
    typedef vtkm::cont::ArrayHandle<FieldType,StorageTag> FieldHandle;

    typedef vtkm::Vec<CoordinateType,3> Vec3;
    typedef vtkm::cont::ArrayHandle<Vec3> Vec3Handle;
    typedef std::vector<Vec3Handle> Vec3HandleVec;

    RunOverIsocontourSetFunctor(const FieldVec& isovalues,
                                const CellSetType& cellSet,
                                const vtkm::cont::CoordinateSystem& coords,
                                const FieldHandle& isoField,
                                Vec3HandleVec& vertices,
                                Vec3HandleVec& normals,
                                FieldHandleVec& interpolationWeights,
                                IdHandleVec& interpolationLowIds,
                                IdHandleVec& interpolationHighIds)
    {
      if (isovalues.size() == NumberOfIsovalues)
        {
        ::vtkm::worklet::internal::IsosurfaceFilterHexahedra<FieldType,
          DeviceAdapter,NumberOfIsovalues> filter;
        filter.template Run<CellSetType,StorageTag,
          CoordinateType>(isovalues,
                          cellSet,
                          coords,
                          isoField,
                          vertices,
                          normals,
                          interpolationWeights,
                          interpolationLowIds,
                          interpolationHighIds);
        }
      else
        RunOverIsocontourSetFunctor<CellSetType,StorageTag,
          CoordinateType,NumberOfIsovalues+1>(isovalues,
                                              cellSet,
                                              coords,
                                              isoField,
                                              vertices,
                                              normals,
                                              interpolationWeights,
                                              interpolationLowIds,
                                              interpolationHighIds);
    }
  };

  template<class CellSetType,typename StorageTag,typename CoordinateType>
  class RunOverIsocontourSetFunctor<CellSetType,StorageTag,CoordinateType,
    MaxNumberOfIsovalues>
  {
  public:
    typedef vtkm::cont::ArrayHandle<FieldType,StorageTag> FieldHandle;

    typedef vtkm::Vec<CoordinateType,3> Vec3;
    typedef vtkm::cont::ArrayHandle<Vec3> Vec3Handle;
    typedef std::vector<Vec3Handle> Vec3HandleVec;

    RunOverIsocontourSetFunctor(const FieldVec&,
                     const CellSetType&,
                     const vtkm::cont::CoordinateSystem&,
                     const FieldHandle&,
                     Vec3HandleVec&,
                     Vec3HandleVec&,
                     FieldHandleVec&,
                     IdHandleVec&,
                     IdHandleVec&)
    {
      return;
    }
  };

  template<typename ArrayHandleIn, typename ArrayHandleOut,IsovalueCount NumberOfIsovalues=1>
  class MapOntoIsocontourSetFunctor
  {
  public:
    MapOntoIsocontourSetFunctor(const ArrayHandleIn& fieldIn,
                                const FieldHandleVec& interpolationWeights,
                                const IdHandleVec& interpolationLowIds,
                                const IdHandleVec& interpolationHighIds,
                                std::vector<ArrayHandleOut>& fieldOut)
    {
      if (fieldOut.size() == NumberOfIsovalues)
        {
        ::vtkm::worklet::internal::IsosurfaceFilterHexahedra<FieldType,
          DeviceAdapter,NumberOfIsovalues> filter;
        filter.template MapFieldOntoIsosurfaces<ArrayHandleIn,
          ArrayHandleOut>(fieldIn,
                          interpolationWeights,
                          interpolationLowIds,
                          interpolationHighIds,
                          fieldOut);
        }
      else
        MapOntoIsocontourSetFunctor<ArrayHandleIn,ArrayHandleOut,
          NumberOfIsovalues+1>(fieldIn,
                               interpolationWeights,
                               interpolationLowIds,
                               interpolationHighIds,
                               fieldOut);
    }
  };

  template<typename ArrayHandleIn, typename ArrayHandleOut>
  class MapOntoIsocontourSetFunctor<ArrayHandleIn,ArrayHandleOut,MaxNumberOfIsovalues>
  {
  public:
    MapOntoIsocontourSetFunctor(const ArrayHandleIn&,
                                const FieldHandleVec&,
                                const IdHandleVec&,
                                const IdHandleVec&,
                                std::vector<ArrayHandleOut>&)
    {
      return;
    }
  };

public:
  template<typename StorageTag,typename CoordinateType>
  void Run(const FieldVec& isovalues,
           const vtkm::cont::DataSet& dataSet,
           const vtkm::cont::ArrayHandle<FieldType,StorageTag>& isoField,
           std::vector<vtkm::cont::ArrayHandle<vtkm::Vec<CoordinateType,3> > >& vertices,
           std::vector<vtkm::cont::ArrayHandle<vtkm::Vec<CoordinateType,3> > >& normals)
  {
    return Run(isovalues,
               dataSet.GetCellSet(),
               dataSet.GetCoordinateSystem(),
               isoField,
               vertices,
               normals);
  }

  template<typename CellSetList,typename StorageTag, typename CoordinateType>
  void Run(const FieldVec& isovalues,
           const vtkm::cont::DynamicCellSetBase<CellSetList>& cellSet,
           const vtkm::cont::CoordinateSystem& coordinateSystem,
           const vtkm::cont::ArrayHandle<FieldType,StorageTag>& isoField,
           std::vector<vtkm::cont::ArrayHandle<vtkm::Vec<CoordinateType,3> > >& vertices,
           std::vector<vtkm::cont::ArrayHandle<vtkm::Vec<CoordinateType,3> > >& normals)
  {
    typedef RunOnCellTypeFunctor<StorageTag,CoordinateType> Run;
    Run run(this,isovalues,coordinateSystem,isoField,vertices,normals);
    return cellSet.CastAndCall(run);
  }

  template<class CellSetType,typename StorageTag,typename CoordinateType>
  void Run(const FieldVec& isovalues,
           const CellSetType& cellSet,
           const vtkm::cont::CoordinateSystem& coords,
           const vtkm::cont::ArrayHandle<FieldType,StorageTag>& isoField,
           std::vector<vtkm::cont::ArrayHandle<vtkm::Vec<CoordinateType,3> > >& vertices,
           std::vector<vtkm::cont::ArrayHandle<vtkm::Vec<CoordinateType,3> > >& normals)
  {
    IsovalueCount nIsovalues = isovalues.size();
    // NB: Cannot call resize to increase the lengths of vectors of array
    // handles! You will end up with a vector of smart pointers to the same
    // array instance. A specialization of std::allocator<> for array handles
    // should be created.
    typedef vtkm::cont::ArrayHandle<FieldType> FieldHandle;
    for (unsigned iso=this->InterpolationWeights.size();iso<nIsovalues;iso++)
      {
      this->InterpolationWeights.push_back(FieldHandle());
      this->InterpolationLowIds.push_back(IdHandle());
      this->InterpolationHighIds.push_back(IdHandle());
      }
    // now that the vectors are guaranteed to be at least as long as we require,
    // we can use resize to shrink them
    this->InterpolationWeights.resize(nIsovalues);
    this->InterpolationLowIds.resize(nIsovalues);
    this->InterpolationHighIds.resize(nIsovalues);

    typedef vtkm::cont::ArrayHandle<vtkm::Vec<CoordinateType,3> > CoordHandle;
    for (unsigned iso=vertices.size();iso<nIsovalues;iso++)
      vertices.push_back(CoordHandle());
    vertices.resize(nIsovalues);

    for (unsigned iso=normals.size();iso<nIsovalues;iso++)
      normals.push_back(CoordHandle());
    normals.resize(nIsovalues);

    RunOverIsocontourSetFunctor<CellSetType,StorageTag,
      CoordinateType>(isovalues,
                      cellSet,
                      coords,
                      isoField,
                      vertices,
                      normals,
                      this->InterpolationWeights,
                      this->InterpolationLowIds,
                      this->InterpolationHighIds);
  }

  template<typename Field, typename StorageTag>
  void MapFieldOntoIsosurfaces(const vtkm::cont::ArrayHandle<Field,StorageTag>& fieldIn,
                               std::vector<vtkm::cont::ArrayHandle< Field> >& fieldOut)
{
  IsovalueCount nIsovalues = InterpolationWeights.size();
  // NB: Cannot call resize to increase the lengths of vectors of array
  // handles! You will end up with a vector of smart pointers to the same
  // array instance. A specialization of std::allocator<> for array handles
  // should be created.
  typedef vtkm::cont::ArrayHandle<Field> FieldHandle;
  // for (unsigned iso=fieldOut.size();iso<nIsovalues;iso++)
  //   fieldOut.push_back(FieldHandle());
  // fieldOut.resize(nIsovalues);

  MapOntoIsocontourSetFunctor<Field,StorageTag,
    FieldHandle::StorageTag>(fieldIn,
                             this->InterpolationWeights,
                             this->InterpolationLowIds,
                             this->InterpolationHighIds,
                             fieldOut);
}

  template<typename ArrayHandleIn, typename ArrayHandleOut>
  void MapFieldOntoIsosurfaces(const ArrayHandleIn& fieldIn,
                               std::vector<ArrayHandleOut>& fieldOut)
{
  MapOntoIsocontourSetFunctor<ArrayHandleIn,
                              ArrayHandleOut>(fieldIn,
                                              this->InterpolationWeights,
                                              this->InterpolationLowIds,
                                              this->InterpolationHighIds,
                                              fieldOut);
}

protected:
    FieldHandleVec InterpolationWeights;
    IdHandleVec    InterpolationLowIds;
    IdHandleVec    InterpolationHighIds;
};

}
} // namespace vtkm::worklet

#endif // vtk_m_worklet_IsosurfaceHexahedra_h
