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

#include "PyFRData.h"

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/DeviceAdapter.h>

#include <vtkm/cont/CellSetSingleType.h>
#include <vtkm/cont/ArrayHandlePermutation.h>
#include <vtkm/worklet/DispatcherMapField.h>
#include <vtkm/worklet/WorkletMapField.h>

namespace {

PyFRData::Vec3ArrayHandle
make_Vec3Handle(const vtkm::cont::CoordinateSystem& coodSys)
{
  return coodSys.GetData().CastToArrayHandle(
    PyFRData::Vec3ArrayHandle::ValueType(),
    PyFRData::Vec3ArrayHandle::StorageTag());
}

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

namespace vtkm {
namespace worklet {

namespace mergepoints {

template <typename DeviceTag>
struct PointLess
{
  typedef vtkm::cont::ArrayHandle<vtkm::Vec<FPType, 3>> PointHandle;
  typedef typename PointHandle::template ExecutionTypes<DeviceTag>::PortalConst
    PointHandleType;

  PointLess(PointHandleType p)
    : Portal(p)
  {
  }

  VTKM_EXEC_CONT_EXPORT bool operator()(
    const vtkm::Id& x, const vtkm::Id& y) const
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

template <typename DeviceTag>
struct PointEqual
{
  typedef vtkm::cont::ArrayHandle<vtkm::Vec<FPType, 3>> PointHandle;
  typedef typename PointHandle::template ExecutionTypes<DeviceTag>::PortalConst
    PointHandleType;

  PointEqual(PointHandleType p)
    : Portal(p)
  {
  }

  VTKM_EXEC_CONT_EXPORT bool operator()(
    const vtkm::Id& x, const vtkm::Id& y) const
  {
    const vtkm::Vec<FPType, 3> a = this->Portal.Get(x);
    const vtkm::Vec<FPType, 3> b = this->Portal.Get(y);
    return a == b;
  }

  PointHandleType Portal;
};
}

class MergePoints
{
public:
  template <typename DeviceTag>
  class RunOnCellTypeFunctor
  {
  public:
    RunOnCellTypeFunctor(MergePoints* filter, const vtkm::cont::DataSet& input,
      vtkm::cont::DataSet& output)
      : Input(input)
      , Output(output)
    {
    }

    template <typename CellSetType>
    void operator()(const CellSetType& cellSet) const
    {
      return this->Filter->Run(cellSet, this->Input, this->Output, DeviceTag());
    }

  private:
    MergePoints* Filter;
    const vtkm::cont::DataSet& Input;
    vtkm::cont::DataSet& Output;
  };

  template <typename CellSetList, typename DeviceTag>
  void Run(const vtkm::cont::DynamicCellSetBase<CellSetList>& cellSet,
    const vtkm::cont::DataSet& input, vtkm::cont::DataSet& output, DeviceTag)
  {
    RunOnCellTypeFunctor<DeviceTag> run(this, input, output);
    cellSet.CastAndCall(run);
  }

  template <typename StorageTag, typename DeviceTag>
  void Run(const vtkm::cont::CellSetSingleType<StorageTag>& cellSet,
    const vtkm::cont::DataSet& input, vtkm::cont::DataSet& output,
    DeviceTag tag)
  {
    typedef vtkm::cont::DeviceAdapterAlgorithm<DeviceTag> Algorithms;
    // Task is to merge exactly duplicate points. This is done
    // by doing the following:
    // 1. grab the topology info
    // 2. copy the topology
    // 3. sort && unique the copy based on the points and add the reduced
    //    points as the coordinate system to the output
    // 4. run lower bounds to determine what index holds the 'merged' index,
    //    which will build our new connectivity array
    // 5. do a reduction on all the fields like we did for the topology

    // 1&&2
    vtkm::cont::ArrayHandle<vtkm::Id> mergedLookup;
    Algorithms::Copy(
      cellSet.GetConnectivityArray(
        vtkm::TopologyElementTagPoint(), vtkm::TopologyElementTagCell()),
      mergedLookup);

    // 3
    // The sort needs take the physical point locations into consideration
    vtkm::cont::CoordinateSystem coordField = input.GetCoordinateSystem();
    PyFRData::Vec3ArrayHandle points = make_Vec3Handle(coordField);
    mergepoints::PointLess<DeviceTag> pointLess(points.PrepareForInput(tag));
    mergepoints::PointEqual<DeviceTag> pointEqual(points.PrepareForInput(tag));

    // std::cout << "Merge Points info: \n ";
    // std::cout << "  input number of points: " << points.GetNumberOfValues()
    //           << "\n ";
    Algorithms::Sort(mergedLookup, pointLess);
    Algorithms::Unique(mergedLookup, pointEqual);
    // std::cout << "  number of final points: "
    //           << mergedLookup.GetNumberOfValues();
    // std::cout << std::endl;

    // Add the reduced points as the new coordinate system
    vtkm::cont::ArrayHandle<vtkm::Vec<FPType, 3>> output_points;
    Algorithms::Copy(
      vtkm::cont::make_ArrayHandlePermutation(mergedLookup, points),
      output_points);
    output.AddCoordinateSystem(
      vtkm::cont::CoordinateSystem("coordinates", 1, output_points));

    // 4
    vtkm::cont::ArrayHandle<vtkm::Id> outputConnectivity;
    Algorithms::LowerBounds(mergedLookup,
      cellSet.GetConnectivityArray(
        vtkm::TopologyElementTagPoint(), vtkm::TopologyElementTagCell()),
      outputConnectivity, pointLess);

    vtkm::cont::CellSetSingleType<> outputCellSet(
      vtkm::CellShapeTagHexahedron(),
      output_points.GetNumberOfValues(),
      "cells");
    outputCellSet.Fill(outputConnectivity);
    output.AddCellSet(outputCellSet);

    // 5
    // now we need to do the same for the point fields
    for (vtkm::IdComponent i = 0; i < input.GetNumberOfFields(); i++)
    {
      vtkm::cont::Field field = input.GetField(PyFRData::FieldName(i));
      vtkm::cont::ArrayHandle<FPType> outputHandle;

      //velocity_magnitude is a special case
      if(PyFRData::FieldName(i) == "velocity_magnitude")
      {
      PyFRData::ScalarDataArrayHandle inputHandle =
        make_ScalarHandle(field);
      // we want to use the mergedLookup as a permutation to the copy
      Algorithms::Copy(
        vtkm::cont::make_ArrayHandlePermutation(mergedLookup, inputHandle),
        outputHandle);
      }
      else
      {
      PyFRData::CatalystMappedDataArrayHandle inputHandle =
        make_CatalystHandle(field);
      // we want to use the mergedLookup as a permutation to the copy
      Algorithms::Copy(
        vtkm::cont::make_ArrayHandlePermutation(mergedLookup, inputHandle),
        outputHandle);
      }

      vtkm::cont::Field outputField(field.GetName(), field.GetOrder(),
        field.GetAssociation(), outputHandle);
      output.AddField(outputField);
    }
  }
};
}
}

#endif