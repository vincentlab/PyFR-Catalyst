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

#ifndef vtk_m_worklet_MergePoints_h
#define vtk_m_worklet_MergePoints_h

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/DeviceAdapter.h>

#include <vtkm/cont/CellSetSingleType.h>
#include <vtkm/worklet/DispatcherMapField.h>
#include <vtkm/worklet/WorkletMapField.h>

#include <vtkm/exec/ExecutionWholeArray.h>

#include "PyFRData.h"

namespace vtkm {
namespace worklet {

namespace mergepoints {

template <typename DeviceTag> struct PointLess
{
  typedef vtkm::cont::ArrayHandle< vtkm::Vec<FPType, 3> > PointHandle;
  typedef typename PointHandle::template ExecutionTypes<DeviceTag>::PortalConst
      PointHandleType;

  PointLess(PointHandleType p) : Portal(p)
  {
  }

  VTKM_EXEC_CONT_EXPORT bool operator()(const vtkm::Id& x,
                                        const vtkm::Id& y) const
  {
    const vtkm::Vec<FPType, 3> a = this->Portal.Get(x);
    const vtkm::Vec<FPType, 3> b = this->Portal.Get(y);
    if (a[0] < b[0])
    {
      return true;
    }
    else if (a[0] == b[0])
    {
      if (a[1] < b[1])
      {
        return true;
      }
      else if ((a[1] == b[1]) && (a[2] < b[2]))
      {
        return true;
      }
    }
    return false;
  }
  PointHandleType Portal;
};

template <typename DeviceTag> struct PointEqual
{
  typedef vtkm::cont::ArrayHandle< vtkm::Vec<FPType, 3> > PointHandle;
  typedef typename PointHandle::template ExecutionTypes<DeviceTag>::PortalConst
      PointHandleType;

  PointEqual(PointHandleType p) : Portal(p)
  {
  }

  VTKM_EXEC_CONT_EXPORT bool operator()(const vtkm::Id& x,
                                        const vtkm::Id& y) const
  {
    const vtkm::Vec<FPType, 3> a = this->Portal.Get(x);
    const vtkm::Vec<FPType, 3> b = this->Portal.Get(y);
    return a == b;
  }

  PointHandleType Portal;
};

struct FillTopology : public vtkm::worklet::WorkletMapField
{
  typedef void ControlSignature(FieldIn<IdType> inConn,
                                FieldOut<IdType> outConn,
                                ExecObject reducedConn);
  typedef void ExecutionSignature(_1, _2, _3);

  template <typename FieldIn, typename FieldOut, typename WholeConn>
  VTKM_EXEC_EXPORT void operator()(const FieldIn& inConn, FieldOut& outConn,
                                   WholeConn reducedConn) const
  {
    outConn = reducedConn.Get(inConn);
  }
};
}

class MergePoints
{
public:
  template <typename PointType, typename DeviceTag> class RunOnCellTypeFunctor
  {
  public:
    RunOnCellTypeFunctor(MergePoints* filter, const PointType& points,
                         vtkm::cont::DataSet& output)
      : Points(points), Output(output)
    {
    }

    template <typename CellSetType>
    void operator()(const CellSetType& cellSet) const
    {
      return this->Filter->Run(cellSet, this->Points, this->Output,
                               DeviceTag());
    }

  private:
    MergePoints* Filter;
    const PointType& Points;
    vtkm::cont::DataSet& Output;
  };

  template <typename CellSetList, typename PointType, typename DeviceTag>
  void Run(const vtkm::cont::DynamicCellSetBase<CellSetList>& cellSet,
           const PointType& points, vtkm::cont::DataSet& output, DeviceTag)
  {
    RunOnCellTypeFunctor<PointType, DeviceTag> run(this, points, output);
    return cellSet.CastAndCall(run);
  }

  template <typename StorageTag, typename PointType, typename DeviceTag>
  void Run(const vtkm::cont::CellSetSingleType<StorageTag>& cellSet,
           const PointType& points, vtkm::cont::DataSet& output, DeviceTag tag)
  {
    typedef vtkm::cont::DeviceAdapterAlgorithm<DeviceTag> Algorithms;
    typedef vtkm::exec::ExecutionWholeArrayConst<
        vtkm::Id, vtkm::cont::StorageTagBasic, DeviceTag>
        ExecWholeArray;
    // Task is to merge exactly duplicate points. This is done
    // by doing the following:
    // 1. grab the topology info
    // 2. copy the topology
    // 3. sort && unique the copy based on the points
    // 4. run lower bounds to determine what index holds the 'merged' index
    // 5. run a copy worklet to copy the 'merged' index value back into the
    // connectivity

    // 1&&2
    vtkm::cont::ArrayHandle<vtkm::Id> reducedConn;
    Algorithms::Copy(
        cellSet.GetConnectivityArray(vtkm::TopologyElementTagPoint(),
                                     vtkm::TopologyElementTagCell()),
        reducedConn);

    // 3
    // The sort needs take the physical point locations into consideration
    mergepoints::PointLess<DeviceTag> pointLess(
        points.PrepareForInput(tag));
    mergepoints::PointEqual<DeviceTag> pointEqual(
        points.PrepareForInput(tag));
    Algorithms::Sort(reducedConn, pointLess);
    Algorithms::Unique(reducedConn, pointEqual);

    // 4
    vtkm::cont::ArrayHandle<vtkm::Id> temp;
    Algorithms::LowerBounds(reducedConn, cellSet.GetConnectivityArray(
                                             vtkm::TopologyElementTagPoint(),
                                             vtkm::TopologyElementTagCell()),
                            temp);

    // 5
    // we need to pass temp, whole array reducedConn, and outputConnectivity
    vtkm::cont::ArrayHandle<vtkm::Id> outputConnectivity;

    vtkm::worklet::DispatcherMapField<mergepoints::FillTopology, DeviceTag>
        dispatcher;
    dispatcher.Invoke(temp, outputConnectivity, ExecWholeArray(reducedConn));

    vtkm::cont::CellSetSingleType<> outputCellSet(
        vtkm::CellShapeTagHexahedron(), "cells");
    outputCellSet.Fill(outputConnectivity);

    output.AddCoordinateSystem(vtkm::cont::CoordinateSystem("coordinates",
                                                             1,points));
    output.AddCellSet(outputCellSet);
  }
};
}
}

#endif