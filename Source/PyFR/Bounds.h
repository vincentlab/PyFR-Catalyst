#ifndef BOUNDS_H
#define BOUNDS_H

#define BOOST_SP_DISABLE_THREADS

#include <vector>

#include <vtkm/Math.h>

namespace internal
{
template<vtkm::IdComponent NumberOfComponents>
class InputToOutputTypeTransform
{
public:
  typedef vtkm::Vec<vtkm::Float64, NumberOfComponents> ResultType;
  typedef vtkm::Pair<ResultType, ResultType> MinMaxPairType;

  template<typename ValueType>
  VTKM_EXEC_EXPORT
  MinMaxPairType operator()(const ValueType &value) const
  {
    ResultType input;
    for (vtkm::IdComponent i = 0; i < NumberOfComponents; ++i)
    {
      input[i] = static_cast<vtkm::Float64>(
          vtkm::VecTraits<ValueType>::GetComponent(value, i));
    }
    return make_Pair(input, input);
  }
};

template<vtkm::IdComponent NumberOfComponents>
class MinMax
{
public:
  typedef vtkm::Vec<vtkm::Float64, NumberOfComponents> ResultType;
  typedef vtkm::Pair<ResultType, ResultType> MinMaxPairType;

  VTKM_EXEC_EXPORT
  MinMaxPairType operator()(const MinMaxPairType &v1, const MinMaxPairType &v2) const
  {
    MinMaxPairType result;
    for (vtkm::IdComponent i = 0; i < NumberOfComponents; ++i)
    {
      result.first[i] = vtkm::Min(v1.first[i], v2.first[i]);
      result.second[i] = vtkm::Max(v1.second[i], v2.second[i]);
    }
    return result;
  }
};
}

#endif
