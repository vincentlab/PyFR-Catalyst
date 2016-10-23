#include <algorithm>
#include "PyFRContourData.h"

#include "PyFRContour.h"
#include "ColorTable.h"
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


class PyFRContourData::ContourDataImpl
{
public:
  ContourDataImpl()
  {
    TablePreset = ColorTable::BLUETOREDRAINBOW;
    this->Table = make_ColorTable(TablePreset, 0.0, 1.0);
  }

  ColorTable::Preset TablePreset;
  RuntimeColorTable Table;
  std::vector<PyFRContour> Contours;
};

//----------------------------------------------------------------------------
PyFRContourData::PyFRContourData()
{
  this->Impl = new ContourDataImpl;
}

//----------------------------------------------------------------------------
PyFRContourData::~PyFRContourData()
{
  this->Impl->Table.ReleaseResources();
  delete this->Impl;
}

//----------------------------------------------------------------------------
unsigned PyFRContourData::GetNumberOfContours() const
{ return this->Impl->Contours.size(); }
//----------------------------------------------------------------------------
PyFRContour& PyFRContourData::GetContour(int i)
{ return this->Impl->Contours[i]; }
//----------------------------------------------------------------------------
const PyFRContour& PyFRContourData::GetContour(int i) const
{ return this->Impl->Contours[i]; }

//----------------------------------------------------------------------------
void PyFRContourData::SetNumberOfContours(unsigned nContours)
{
  // NB: Cannot call resize to increase the lengths of vectors of array
  // handles (or classes containing them)! You will end up with a vector of
  // smart pointers to the same array instance. A specialization of
  // std::allocator<> for array handles should be created.

  for (unsigned i=this->Impl->Contours.size();i<nContours;i++)
    {
    this->Impl->Contours.push_back(PyFRContour(this->Impl->Table));
    }
  unsigned contourSize = this->Impl->Contours.size();
  for (unsigned i=nContours;i<contourSize;i++)
    {
    this->Impl->Contours.pop_back();
    }
}

//----------------------------------------------------------------------------
unsigned PyFRContourData::GetContourSize(int contour) const
{
  if(contour < this->Impl->Contours.size())
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
    bounds[0] = std::min(bounds[0], b[0]);
    bounds[1] = std::max(bounds[1], b[1]);
    bounds[2] = std::min(bounds[2], b[2]);
    bounds[3] = std::max(bounds[3], b[3]);
    bounds[4] = std::min(bounds[4], b[4]);
    bounds[5] = std::max(bounds[5], b[5]);
    }
}

//----------------------------------------------------------------------------
void PyFRContourData::SetColorPalette(int preset, FPType min, FPType max)
{
  std::cout << "SetColorPalette: " << preset << std::endl;
  this->Impl->Table = make_ColorTable(static_cast<ColorTable::Preset>(preset), min, max);
  this->Impl->TablePreset = static_cast<ColorTable::Preset>(preset);

  for (unsigned i=0;i<this->GetNumberOfContours();i++)
    {
    this->Impl->Contours[i].ChangeColorTable(this->Impl->Table);
    }
}


//----------------------------------------------------------------------------
void PyFRContourData::SetColorPreset(int preset)
{
  this->SetColorPalette(static_cast<ColorTable::Preset>(preset),
                        this->Impl->Table.Min,
                        this->Impl->Table.Max);
}

//----------------------------------------------------------------------------
void PyFRContourData::SetColorRange(FPType min,FPType max)
{
  this->SetColorPalette(this->Impl->TablePreset,
                        min,
                        max);
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
