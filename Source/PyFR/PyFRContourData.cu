#include "PyFRContourData.h"

#include "Bounds.h"

#include <vtkm/Math.h>
#include <vtkm/Pair.h>
#include <vtkm/Types.h>
#include <vtkm/VectorAnalysis.h>

#include <vtkm/cont/cuda/DeviceAdapterCuda.h>
#include <vtkm/cont/DeviceAdapter.h>
#include <vtkm/cont/ArrayHandleTransform.h>

#include <vtkm/cont/ArrayHandleCast.h>
#include <vtkm/opengl/TransferToOpenGL.h>
#include <vtkm/opengl/cuda/internal/TransferToOpenGL.h>

//----------------------------------------------------------------------------
void PyFRContourData::SetNumberOfContours(unsigned nContours)
{
  // NB: Cannot call resize to increase the lengths of vectors of array
  // handles (or classes containing them)! You will end up with a vector of
  // smart pointers to the same array instance. A specialization of
  // std::allocator<> for array handles should be created.

  for (unsigned i=this->Contours.size();i<nContours;i++)
    {
    this->Contours.push_back(PyFRContour(this->Table));
    }
  unsigned contourSize = this->Contours.size();
  for (unsigned i=nContours;i<contourSize;i++)
    {
    this->Contours.pop_back();
    }
}

//----------------------------------------------------------------------------
unsigned PyFRContourData::GetContourSize(int contour) const
{
  if(contour < this->Contours.size())
    return this->GetContour(contour).GetVertices().GetNumberOfValues();
  return 0;
}

//----------------------------------------------------------------------------
void PyFRContourData::ComputeContourBounds(int contour,FPType* bounds) const
{
  typedef ::vtkm::cont::DeviceAdapterTagCuda CudaTag;
  typedef vtkm::cont::DeviceAdapterAlgorithm<CudaTag> Algorithm;
  typedef vtkm::Vec<vtkm::Float64, 3> ResultType;
  typedef vtkm::Pair<ResultType, ResultType> MinMaxPairType;
  typedef PyFRContour::Vec3ArrayHandle ArrayHandleType;

  MinMaxPairType initialValue =
    make_Pair(ResultType(vtkm::Infinity64()),
              ResultType(vtkm::NegativeInfinity64()));

  vtkm::cont::ArrayHandleTransform<MinMaxPairType, ArrayHandleType,
    internal::InputToOutputTypeTransform<3> > input(this->GetContour(contour)
                                                    .GetVertices());

  MinMaxPairType result = Algorithm::Reduce(input, initialValue,
                                            internal::MinMax<3>());

  for (unsigned i=0;i<3;i++)
    {
    bounds[2*i] = result.first[i];
    bounds[2*i+1] = result.second[i];
    }
}

//----------------------------------------------------------------------------
void PyFRContourData::ComputeBounds(FPType* bounds) const
{
  for (unsigned i=0;i<3;i++)
    {
    bounds[2*i] = std::numeric_limits<FPType>::max();
    bounds[2*i+1] = std::numeric_limits<FPType>::min();
    }
  for (unsigned i=0;i<this->GetNumberOfContours();i++)
    {
    FPType b[6];
    this->ComputeContourBounds(i,b);
    for (unsigned j=0;j<3;j++)
      {
      int jj = 2*j;
      bounds[jj] = (bounds[jj] < b[jj] ? bounds[jj] : b[jj]);
      jj++;
      bounds[jj] = (bounds[jj] > b[jj] ? bounds[jj] : b[jj]);
      }
    }
}

//----------------------------------------------------------------------------
void PyFRContourData::SetColorPalette(int preset,FPType min,FPType max)
{
  this->Table.SetRange(min,max);
  this->Table.PresetColorTable(static_cast<ColorTable::Preset>(preset));
  for (unsigned i=0;i<this->GetNumberOfContours();i++)
    {
    this->Contours[i].ChangeColorTable(this->Table);
    }
}

namespace transfer
{

typedef ::vtkm::cont::DeviceAdapterTagCuda CudaTag;

//----------------------------------------------------------------------------
template<typename HandleType>
void to_gl(vtkm::Vec<vtkm::Float64,3>, const HandleType& handle, unsigned int& glHandle)
{
  //make an implicit wrapper to float32 around the float64 array
  vtkm::cont::ArrayHandleCast<vtkm::Vec<vtkm::Float32,3>,HandleType> asF32 =
    vtkm::cont::make_ArrayHandleCast(handle, vtkm::Vec<vtkm::Float32,3>());

  //transfer the array to openGL now as a float32 array
  vtkm::opengl::TransferToOpenGL(asF32, glHandle, CudaTag());
}

//----------------------------------------------------------------------------
template<typename HandleType>
void to_gl(vtkm::Float32, const HandleType& handle, unsigned int& glHandle)
{
  vtkm::opengl::TransferToOpenGL(handle, glHandle, CudaTag());
}

//----------------------------------------------------------------------------
void coords(PyFRContourData* data, int index, unsigned int& glHandle)
{
  to_gl(FPType(), data->GetContour(index).GetVertices(), glHandle);
}

//----------------------------------------------------------------------------
void normals(PyFRContourData* data, int index, unsigned int& glHandle)
{
  to_gl(FPType(), data->GetContour(index).GetNormals(), glHandle);
}

//----------------------------------------------------------------------------
void colors(PyFRContourData* data, int index, unsigned int& glHandle)
{
  //no need to worry about conversion, since this is always Vec4 of uint8's
  vtkm::opengl::TransferToOpenGL( data->GetContour(index).GetColorData(),
                                  glHandle,
                                  CudaTag());
}

} //namespace transfer
