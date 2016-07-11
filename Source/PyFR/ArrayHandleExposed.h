//=============================================================================
//
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2015 Sandia Corporation.
//  Copyright 2015 UT-Battelle, LLC.
//  Copyright 2015 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//
//=============================================================================
#ifndef vtk_m_ArrayHandleExposed_h
#define vtk_m_ArrayHandleExposed_h

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/Storage.h>

namespace vtkm {
namespace cont {

template<typename T, typename StorageTag_ = VTKM_DEFAULT_STORAGE_TAG>
class ArrayHandleExposed : public ArrayHandle<T,StorageTag_>
{
public:
  typedef vtkm::cont::internal::Storage<T,StorageTag_> StorageType;

  VTKM_ARRAY_HANDLE_SUBCLASS(
    ArrayHandleExposed,
    (ArrayHandleExposed<T,StorageTag_>),
    (ArrayHandle<T,StorageTag_>));

  StorageType& Storage()
  {
    this->SyncControlArray();
    if (this->Internals->ControlArrayValid)
      {
      return this->Internals->ControlArray;
      }
    else
      {
      throw vtkm::cont::ErrorControlInternal(
        "ArrayHandle::SyncControlArray did not make control array valid.");
      }
  }
};

} //namespace cont
} //namespace vtkm

#endif //vtk_m_ArrayHandleExposed_h
