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

#ifndef vtk_m_worklet_Gradients_h
#define vtk_m_worklet_Gradients_h

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/DeviceAdapter.h>

#include <vtkm/cont/CellSetSingleType.h>
#include <vtkm/worklet/DispatcherMapField.h>
#include <vtkm/worklet/WorkletMapField.h>

#include <vtkm/exec/ExecutionWholeArray.h>
#include <vtkm/exec/ParametricCoordinates.h>

#include <vtkm/VectorAnalysis.h>

#include "PyFRData.h"

namespace {

PyFRData::Vec3ArrayHandle
make_Vec3Handle(const vtkm::cont::CoordinateSystem& coodSys)
{
  return coodSys.GetData().CastToArrayHandle(
      PyFRData::Vec3ArrayHandle::ValueType(),
      PyFRData::Vec3ArrayHandle::StorageTag());
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

typedef vtkm::worklet::WorkletMapTopology<vtkm::TopologyElementTagCell,
                                          vtkm::TopologyElementTagPoint>
    WorkletMapCellToPoint;

template<typename StorageTag, typename DeviceTag>
struct ComputeGradients : public vtkm::worklet::WorkletMapCellToPoint
{
  typedef vtkm::Vec<vtkm::Id,8> HexTopo;
  typedef vtkm::Vec<FPType,8> GradientField;
  typedef vtkm::Vec< vtkm::Vec<FPType,3>,8> VecField;

  ComputeGradients(vtkm::cont::CellSetSingleType<StorageTag>& cellset,
                   PyFRData::Vec3ArrayHandle coords,
                   PyFRData::ScalarDataArrayHandle density,
                   PyFRData::ScalarDataArrayHandle velu,
                   PyFRData::ScalarDataArrayHandle velv,
                   PyFRData::ScalarDataArrayHandle velw)
    : Topo(cellset.PrepareForInput(DeviceTag(), vtkm::TopologyElementTagPoint(),
                                   vtkm::TopologyElementTagCell())),
      Coordinates(coords.PrepareForInput(DeviceTag())),
      Density(density.PrepareForInput(DeviceTag())),
      VelocityU(velu.PrepareForInput(DeviceTag())),
      VelocityV(velv.PrepareForInput(DeviceTag())),
      VelocityW(velw.PrepareForInput(DeviceTag()))
  {
  }

  typedef void ControlSignature(TopologyIn topology,
                                FieldOut<Scalar> d_gradient_mag,
                                FieldOut<Scalar> qcriterion);
  typedef void ExecutionSignature(FromCount, FromIndices, WorkIndex, _2, _3);
  typedef _1 InputDomain;

  template <typename FromIndexType>
  VTKM_EXEC_EXPORT void operator()(const vtkm::IdComponent& numCells,
                                   const FromIndexType& cellIds,
                                   const vtkm::Id& pointId,
                                   FPType& d_gradient_mag,
                                   FPType& qcriterion) const
  {
    // if this point is not used we don't need to compute anything
    if (numCells == 0)
    {
      d_gradient_mag = 0;
      qcriterion = 0;
      return;
    }

    vtkm::Vec<FPType, 3> d_gradient(0.0);
    vtkm::Vec<FPType, 9> v_gradient(0.0);

    for (vtkm::IdComponent i = 0; i < numCells; ++i)
    {
      vtkm::Id cellId = cellIds[i];

      // fetch the topology of this cellId and determine which point index
      // for this cell our point is
      const HexTopo topo = this->GetTopo(cellId);
      const vtkm::IdComponent pointIndexForCell =
          this->GetPointIndexForCell(topo, pointId);

      // compute the parametric coordinates for the current point
      vtkm::Vec<FPType, 3> pcoords;
      vtkm::exec::ParametricCoordinatesPoint(
          8, pointIndexForCell, pcoords, vtkm::CellShapeTagHexahedron(), *this);

      // compute the coordinates of the current cell
      const VecField wcoords = this->GetPointsOfCell(topo);

      // compute the density derivative for this cell
      {
        const GradientField density = this->GetDensityValues(topo);
        const vtkm::Vec<FPType, 3> temp = vtkm::exec::CellDerivative(
            density, wcoords, pcoords, vtkm::CellShapeTagHexahedron(), *this);
        d_gradient[0] += temp[0];
        d_gradient[1] += temp[1];
        d_gradient[2] += temp[2];
      }

      // compute the velocity derivative for this cell
      {
        GradientField velocity_t = this->GetVelocityUValues(topo);
        vtkm::Vec<FPType, 3> temp =
            vtkm::exec::CellDerivative(velocity_t, wcoords, pcoords,
                                       vtkm::CellShapeTagHexahedron(), *this);

        v_gradient[0] += temp[0];
        v_gradient[1] += temp[1];
        v_gradient[2] += temp[2];

        velocity_t = this->GetVelocityVValues(topo);
        temp =
            vtkm::exec::CellDerivative(velocity_t, wcoords, pcoords,
                                       vtkm::CellShapeTagHexahedron(), *this);

        v_gradient[3] += temp[0];
        v_gradient[4] += temp[1];
        v_gradient[5] += temp[2];

        velocity_t = this->GetVelocityWValues(topo);
        temp =
            vtkm::exec::CellDerivative(velocity_t, wcoords, pcoords,
                                       vtkm::CellShapeTagHexahedron(), *this);

        v_gradient[6] += temp[0];
        v_gradient[7] += temp[1];
        v_gradient[8] += temp[2];
      }
    }

    //now we need to average out our gradients
    for(std::size_t i=0; i < 3; ++i)
    {
      d_gradient[i] /= float(numCells);
      v_gradient[i*3] /= float(numCells);
      v_gradient[i*3+1] /= float(numCells);
      v_gradient[i*3+2] /= float(numCells);
    }

    // save magnitudes
    d_gradient_mag = vtkm::Magnitude(d_gradient);

    //now we need to compute the Q Criterion and save that out
    {
      FPType t1 =
          ((v_gradient[7] - v_gradient[5]) * (v_gradient[7] - v_gradient[5]) +
           (v_gradient[3] - v_gradient[1]) * (v_gradient[3] - v_gradient[1]) +
           (v_gradient[2] - v_gradient[6]) * (v_gradient[2] - v_gradient[6])) /
          2.0;
      FPType t2 =
          v_gradient[0] * v_gradient[0] + v_gradient[4] * v_gradient[4] +
          v_gradient[8] * v_gradient[8] +
          ((v_gradient[3] + v_gradient[1]) * (v_gradient[3] + v_gradient[1]) +
           (v_gradient[6] + v_gradient[2]) * (v_gradient[6] + v_gradient[2]) +
           (v_gradient[7] + v_gradient[5]) * (v_gradient[7] + v_gradient[5])) /
              2.0;

      //save qcriterion
      qcriterion = (t1 - t2) / 2.0;
    }
  }

  VTKM_EXEC_EXPORT
  HexTopo GetTopo(vtkm::Id cellId) const
  {
    typedef typename TopoObjectType::IndicesType IndicesType;

    HexTopo result;
    IndicesType indices = this->Topo.GetIndices(cellId);

    for (std::size_t i = 0; i < 8; ++i)
    {
      result[i] = indices[i];
    }
    return result;
  }

  VTKM_EXEC_EXPORT
  vtkm::IdComponent GetPointIndexForCell(const HexTopo& topo, vtkm::Id cellId) const
  {
    vtkm::IdComponent result = 0;
    for (std::size_t i = 0; i < 8; ++i)
    {
      if (topo[i] == cellId)
      {
        result = i;
      }
    }
    return result;
  }

  VTKM_EXEC_EXPORT
  VecField GetPointsOfCell(const HexTopo& topo) const
  {
    VecField result;
    this->GetValues(topo,result,this->Coordinates);
    return result;
  }

  VTKM_EXEC_EXPORT
  GradientField GetDensityValues(const HexTopo& topo) const
  {
    GradientField result;
    this->GetValues(topo,result,this->Density);
    return result;
  }

  VTKM_EXEC_EXPORT
  GradientField GetVelocityUValues(const HexTopo& topo) const
  {
    GradientField result;
    this->GetValues(topo,result,this->VelocityU);
    return result;
  }

  VTKM_EXEC_EXPORT
  GradientField GetVelocityVValues(const HexTopo& topo) const
  {
    GradientField result;
    this->GetValues(topo,result,this->VelocityV);
    return result;
  }

  VTKM_EXEC_EXPORT
  GradientField GetVelocityWValues(const HexTopo& topo) const
  {
    GradientField result;
    this->GetValues(topo,result,this->VelocityW);
    return result;
  }

  template<typename T, typename U>
  VTKM_EXEC_EXPORT
  T GetValues(const HexTopo& topo, T& result, const U& portal) const
  {
    for(std::size_t i=0; i < 8; ++i)
      {
      result[i] = portal.Get(topo[i]);
      }
    return result;
  }

private:
  typedef vtkm::cont::CellSetSingleType<StorageTag> ContStorageType;
  typedef typename ContStorageType::template ExecutionTypes<
      DeviceTag, vtkm::TopologyElementTagPoint,
      vtkm::TopologyElementTagCell>::ExecObjectType TopoObjectType;

  typedef typename PyFRData::ScalarDataArrayHandle::template ExecutionTypes<DeviceTag>::PortalConst ScalarPortalType;
  typedef typename PyFRData::Vec3ArrayHandle::template ExecutionTypes<DeviceTag>::PortalConst Vec3PortalType;

  TopoObjectType Topo;
  Vec3PortalType Coordinates;
  ScalarPortalType Density;
  ScalarPortalType VelocityU;
  ScalarPortalType VelocityV;
  ScalarPortalType VelocityW;
};

class Gradients
{
public:
  template <typename DeviceTag> class RunOnCellTypeFunctor
  {
  public:
    RunOnCellTypeFunctor(Gradients* filter, const vtkm::cont::DataSet& input,
                         vtkm::cont::ArrayHandle<FPType>& densityGradientMag,
                         vtkm::cont::ArrayHandle<FPType>& pressureGradientMag,
                         vtkm::cont::ArrayHandle<FPType>& velocityGradientMag,
                         vtkm::cont::ArrayHandle<FPType>& velocityQCriterion,
                         vtkm::cont::DataSet& output)
      : Input(input),
        DensityGradientMag(densityGradientMag),
        PressureGradientMag(pressureGradientMag),
        VelocityGradientMag(velocityGradientMag),
        VelocityQCriterion(velocityQCriterion),
        Output(output)
    {
    }

    template <typename CellSetType>
    void operator()(const CellSetType& cellSet) const
    {
      return this->Filter->Run(cellSet, this->Input, DensityGradientMag,
                               PressureGradientMag, VelocityGradientMag,
                               VelocityQCriterion, this->Output, DeviceTag());
    }

  private:
    Gradients* Filter;
    const vtkm::cont::DataSet& Input;
    vtkm::cont::ArrayHandle<FPType>& DensityGradientMag;
    vtkm::cont::ArrayHandle<FPType>& PressureGradientMag;
    vtkm::cont::ArrayHandle<FPType>& VelocityGradientMag;
    vtkm::cont::ArrayHandle<FPType>& VelocityQCriterion;
    vtkm::cont::DataSet& Output;
  };

  template <typename CellSetList, typename DeviceTag>
  void Run(const vtkm::cont::DynamicCellSetBase<CellSetList>& cellSet,
           const vtkm::cont::DataSet& input,
           vtkm::cont::ArrayHandle<FPType>& densityGradientMag,
           vtkm::cont::ArrayHandle<FPType>& pressureGradientMag,
           vtkm::cont::ArrayHandle<FPType>& velocityGradientMag,
           vtkm::cont::ArrayHandle<FPType>& velocityQCriterion,
           vtkm::cont::DataSet& output, DeviceTag)
  {
    RunOnCellTypeFunctor<DeviceTag> run(
        this, input, densityGradientMag, pressureGradientMag,
        velocityGradientMag, velocityQCriterion, output);
    return cellSet.CastAndCall(run);
  }

  //we need to create a copy of the cellset here!
  //this is needed as we are going to possible recompute the
  //member variable NumberOfPoints
  template <typename StorageTag, typename DeviceTag>
  void Run(vtkm::cont::CellSetSingleType<StorageTag> cellSet,
           const vtkm::cont::DataSet& input,
           vtkm::cont::ArrayHandle<FPType>& densityGradientMag,
           vtkm::cont::ArrayHandle<FPType>& pressureGradientMag,
           vtkm::cont::ArrayHandle<FPType>& velocityGradientMag,
           vtkm::cont::ArrayHandle<FPType>& velocityQCriterion,
           vtkm::cont::DataSet& output,
           DeviceTag tag)
  {
    typedef vtkm::cont::DeviceAdapterAlgorithm<DeviceTag> Algorithms;

    vtkm::cont::Timer<DeviceTag> timer;

    //We need to compute the gradient of all the data.
    vtkm::cont::CoordinateSystem coordField = input.GetCoordinateSystem();
    vtkm::cont::Field densityField = input.GetField("density");
    vtkm::cont::Field veluField = input.GetField("velocity_u");
    vtkm::cont::Field velvField = input.GetField("velocity_v");
    vtkm::cont::Field velwField = input.GetField("velocity_w");

    PyFRData::Vec3ArrayHandle coords = make_Vec3Handle(coordField);
    PyFRData::ScalarDataArrayHandle density = make_ScalarHandle(densityField);
    PyFRData::ScalarDataArrayHandle velu = make_ScalarHandle(veluField);
    PyFRData::ScalarDataArrayHandle velv = make_ScalarHandle(velvField);
    PyFRData::ScalarDataArrayHandle velw = make_ScalarHandle(velwField);

    ComputeGradients<StorageTag, DeviceTag> worklet(
        cellSet, coords, density, velu, velv, velw);

    vtkm::worklet::DispatcherMapTopology<
        ComputeGradients<StorageTag, DeviceTag>, DeviceTag>
        dispatcher(worklet);
    dispatcher.Invoke(input.GetCellSet(), densityGradientMag,
                      velocityQCriterion);
  }
};
}
}

#endif