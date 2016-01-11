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

#ifndef vtk_m_worklet_CrinkleClip_h
#define vtk_m_worklet_CrinkleClip_h

#include <cassert>
#include <stdlib.h>
#include <stdio.h>

#include <vtkm/BinaryPredicates.h>

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

namespace vtkm {
namespace worklet {

template <typename CellSetType>
struct CrinkleClipTraits
{
  typedef vtkm::cont::CellSetPermutation<vtkm::cont::ArrayHandle<vtkm::Id>,
    CellSetType> CellSet;
};

/// \brief Compute the isosurface for a uniform grid data set
template <typename DeviceAdapter>
class CrinkleClip
{
protected:
  typedef vtkm::cont::DataSet DataSet;
  typedef vtkm::cont::ArrayHandle<vtkm::Id> IdHandle;
  typedef vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapter> Algorithm;

public:
  template <class BinaryPredicate>
  class ClassifyCell : public vtkm::worklet::WorkletMapPointToCell
  {
  public:
    typedef void ControlSignature(TopologyIn topology,
                                  FieldInFrom<Scalar> field,
                                  FieldInFrom<Scalar> clipField,
                                  FieldOut<IdType> validCell);
    typedef void ExecutionSignature(_2, _3, FromCount, _4);
    typedef _1 InputDomain;

    const BinaryPredicate& BinaryOp;

    VTKM_CONT_EXPORT
    ClassifyCell(const BinaryPredicate& binaryOp) : BinaryOp(binaryOp) {}

    template<typename FieldVecType,typename ClipFieldVecType>
    VTKM_EXEC_EXPORT
    void operator()(const FieldVecType &field,
                    const ClipFieldVecType &clipField,
                    vtkm::Id count,
                    vtkm::Id& validCell) const
    {
      validCell = 0;

      for (vtkm::IdComponent i = 0; i < count; ++i)
        {
        if (this->BinaryOp(field[i],clipField[i]))
          {
          validCell = 1;
          }
        }
    }
  };

private:
  template<typename FieldHandle,typename ClipFieldHandle,
  typename BinaryPredicate>
  class RunOnCellTypeFunctor
  {
  public:
    typedef CrinkleClip<DeviceAdapter> CrinkleClip;

    RunOnCellTypeFunctor(CrinkleClip* filter,
                         const FieldHandle& field,
                         const ClipFieldHandle& clipField,
                         const BinaryPredicate& binaryOp,
                         const vtkm::cont::CoordinateSystem& coords,
                         vtkm::cont::DataSet& output) : Filter(filter),
                                                        Field(field),
                                                        ClipField(clipField),
                                                        BinaryOp(binaryOp),
                                                        Coords(coords),
                                                        Output(output) {}

    template<typename CellSetType>
    void operator() (const CellSetType& cellSet) const
    {
      return this->Filter->Run(this->Field,
                               this->ClipField,
                               this->BinaryOp,
                               cellSet,
                               this->Coords,
                               this->Output);
    }

  private:
    CrinkleClip* Filter;
    const FieldHandle& Field;
    const ClipFieldHandle& ClipField;
    const BinaryPredicate& BinaryOp;
    const vtkm::cont::CoordinateSystem& Coords;
    vtkm::cont::DataSet& Output;
  };

public:
  template<typename FieldHandle,typename ClipFieldHandle,
  typename BinaryPredicate>
  void Run(const FieldHandle& field,
           const ClipFieldHandle& clipField,
           const BinaryPredicate& binaryOp,
           const vtkm::cont::DataSet& input,
           vtkm::cont::DataSet& output)
  {
    return Run(field,
               clipField,
               binaryOp,
               input.GetCellSet(),
               input.GetCoordinateSystem(),
               output);
  }

  template<class CellSetList,typename FieldHandle,typename ClipFieldHandle,
    typename BinaryPredicate>
  void Run(const FieldHandle& field,
           const ClipFieldHandle& clipField,
           const BinaryPredicate& binaryOp,
           const vtkm::cont::DynamicCellSetBase<CellSetList>& cellSet,
           const vtkm::cont::CoordinateSystem& coords,
           vtkm::cont::DataSet& output)
  {
    RunOnCellTypeFunctor<FieldHandle,ClipFieldHandle,
      BinaryPredicate> run(this,
                           field,
                           clipField,
                           binaryOp,
                           coords,
                           output);
    return cellSet.CastAndCall(run);
  }

  template<class CellSetType,typename FieldHandle,typename ClipFieldHandle,
    typename BinaryPredicate>
  void Run(const FieldHandle& field,
           const ClipFieldHandle& clipField,
           const BinaryPredicate& binaryOp,
           const CellSetType& cellSet,
           const vtkm::cont::CoordinateSystem& coords,
           vtkm::cont::DataSet& output)
  {
    IdHandle cellCount;

    ClassifyCell<BinaryPredicate> classifyCell(binaryOp);
    DispatcherMapTopology<ClassifyCell<BinaryPredicate>,
      DeviceAdapter>(classifyCell).Invoke(cellSet,
                                          field,
                                          clipField,
                                          cellCount);

    vtkm::Id numberOfCells = Algorithm::ScanInclusive(cellCount,cellCount);

    vtkm::cont::ArrayHandleCounting<vtkm::Id> validCellCount(0, 1,
                                                             numberOfCells);

    IdHandle validCellIndices;
    Algorithm::UpperBounds(cellCount,validCellCount,validCellIndices);

    typename CrinkleClipTraits<CellSetType>::CellSet
      outputCellSet(validCellIndices,cellSet);

    output.AddCoordinateSystem(coords);
    output.AddCellSet(outputCellSet);
  }
};

}
} // namespace vtkm::worklet

#endif // vtk_m_worklet_IsosurfaceHexahedra_h
