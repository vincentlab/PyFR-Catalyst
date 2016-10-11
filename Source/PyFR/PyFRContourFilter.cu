#define BOOST_SP_DISABLE_THREADS
#include <algorithm>
#include <cfloat>
#include <vtkm/cont/cuda/DeviceAdapterCuda.h>
#include <vtkm/Math.h>
#include "PyFRContourFilter.h"

#include "CrinkleClip.h"
#include "PyFRData.h"
#include "PyFRContourData.h"

#include <vtkm/cont/Timer.h>

template<typename DeviceAdapter>
class tjfMinMax {
public:
  template<typename T>
  struct minFunctor {
    VTKM_EXEC_EXPORT T
    operator()(const T& a, const T& b) const {
      return vtkm::Min(a, b);
    }
  };
  template<typename T>
  struct maxFunctor {
    VTKM_EXEC_EXPORT T
    operator()(const T& a, const T& b) const {
      return vtkm::Max(a, b);
    }
  };

  template<typename T> void
  Run(PyFRData::ScalarDataArrayHandle array, T* mn, T* mx) {
    typedef typename vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapter>
      DeviceAlgorithms;
    *mn = DeviceAlgorithms::Reduce(array, T(FLT_MAX), minFunctor<T>());
    *mx = DeviceAlgorithms::Reduce(array, T(FLT_MIN), maxFunctor<T>());
  }
};

struct ContourFilterCellSets
  : vtkm::ListTagBase<
      PyFRData::CellSet,
      vtkm::worklet::CrinkleClipTraits<typename PyFRData::CellSet>::CellSet
    > {};

//----------------------------------------------------------------------------
PyFRContourFilter::PyFRContourFilter() : ContourField(0)
{
  minmax[0] = FLT_MIN;
  minmax[1] = FLT_MAX;
}

//----------------------------------------------------------------------------
PyFRContourFilter::~PyFRContourFilter()
{
}

//----------------------------------------------------------------------------
void PyFRContourFilter::operator()(PyFRData* input,
                                   PyFRContourData* output)
{
  typedef std::vector<vtkm::cont::ArrayHandle<vtkm::Vec<FPType,3> > >
    Vec3HandleVec;
  typedef std::vector<FPType> DataVec;

  const vtkm::cont::DataSet& dataSet = input->GetDataSet();
  vtkm::cont::Field contourField =
    dataSet.GetField(PyFRData::FieldName(this->ContourField));
  PyFRData::ScalarDataArrayHandle contourArray = contourField.GetData()
    .CastToArrayHandle(PyFRData::ScalarDataArrayHandle::ValueType(),
                       PyFRData::ScalarDataArrayHandle::StorageTag());

  // compute the range, client can query later via ::Range().
  tjfMinMax<vtkm::cont::DeviceAdapterTagCuda>().
    Run(contourArray, &this->minmax[0], &this->minmax[1]);

  DataVec dataVec;
  Vec3HandleVec verticesVec;
  Vec3HandleVec normalsVec;
  output->SetNumberOfContours(this->ContourValues.size());
  for (unsigned i=0;i<output->GetNumberOfContours();i++)
    {
    dataVec.push_back(this->ContourValues[i]);
    verticesVec.push_back(output->GetContour(i).GetVertices());
    normalsVec.push_back(output->GetContour(i).GetNormals());
    }

  vtkm::cont::Timer<CudaTag> timer;
  isosurfaceFilter.Run(dataVec,
    dataSet.GetCellSet().ResetCellSetList(ContourFilterCellSets()),
    dataSet.GetCoordinateSystem(),
    contourArray,
    verticesVec,
    normalsVec
  );

  std::cout << "time to contour: " << timer.GetElapsedTime() << std::endl;
}
//----------------------------------------------------------------------------
void PyFRContourFilter::MapFieldOntoIsosurfaces(int field,
                                                PyFRData* input,
                                                PyFRContourData* output)
{
  typedef std::vector<PyFRContour::ScalarDataArrayHandle> ScalarDataHandleVec;

  const vtkm::cont::DataSet& dataSet = input->GetDataSet();

  ScalarDataHandleVec scalarDataHandleVec;
  for (unsigned j=0;j<output->GetNumberOfContours();j++)
    {
    output->GetContour(j).SetScalarDataType(field);
    PyFRContour::ScalarDataArrayHandle scalars_out =
      output->GetContour(j).GetScalarData();

    scalarDataHandleVec.push_back(scalars_out);
    }

  vtkm::cont::Field projectedField =
    dataSet.GetField(PyFRData::FieldName(field));

  PyFRData::ScalarDataArrayHandle projectedArray = projectedField.GetData()
    .CastToArrayHandle(PyFRData::ScalarDataArrayHandle::ValueType(),
                       PyFRData::ScalarDataArrayHandle::StorageTag());

  vtkm::cont::Timer<CudaTag> timer;
  isosurfaceFilter.MapFieldOntoIsosurfaces(projectedArray,
                                           scalarDataHandleVec);

  std::cout << "time to map onto contour: " << timer.GetElapsedTime() << std::endl;

}

std::pair<float,float> PyFRContourFilter::Range() const {
  std::pair<float,float> mm;
  mm.first = this->minmax[0];
  mm.second = this->minmax[1];
  return mm;
}
