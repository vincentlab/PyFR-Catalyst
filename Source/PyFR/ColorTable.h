#ifndef COLORTABLE_H
#define COLORTABLE_H

#define BOOST_SP_DISABLE_THREADS

#include <cassert>
#include <cfloat>
#include <vector>
#include <cstdio>

#include <vtkm/Types.h>
#include <vtkm/VecVariable.h>
#include <vtkm/VectorAnalysis.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/cuda/internal/DeviceAdapterTagCuda.h>
#include <vtkm/exec/cuda/internal/ArrayPortalFromThrust.h>
#include <vtkm/cont/ErrorControlBadType.h>

typedef vtkm::Vec<vtkm::UInt8,4> Color;

// Redefine Lerp to avoid subtraction because Color has unsigned elements
VTKM_EXEC_CONT_EXPORT
Color Lerp(const Color& color0,
           const Color& color1,
           const float& weight)
{
  Color result;
  for (vtkm::IdComponent i=0;i<4;i++)
    {
    result[i] = (1. - weight)*color0[i] + weight*color1[i];
    }
  return result;
}

class ColorTable
{
public:
  enum Preset
  {
    COOLTOWARM,
    BLACKBODY,
    BLUETOREDRAINBOW,
    GRAYSCALE,
    GREENWHITELINEAR,
    RUNTIME = -1
  };
};

//exec side representation of a single color table
class RuntimeColorTable : public ColorTable
{
public:
  FPType Min;
  FPType Max;
  vtkm::IdComponent NumberOfColors;

  Color* Palette;
  float* Pivots;

  VTKM_EXEC_CONT_EXPORT
  RuntimeColorTable()
    : Min(0.0)
    , Max(1.0)
    , NumberOfColors(0)
    , Palette(NULL)
    , Pivots(NULL)
  {
  }


  RuntimeColorTable(FPType cmin, FPType cmax,
                    const std::vector<Color>& palette,
                    const std::vector<float>& pivots);

  void ReleaseResources();

  VTKM_EXEC_CONT_EXPORT
  Color operator()(const FPType& value) const
  {
    if (value < this->Pivots[0])
      {
       return this->Palette[0];
      }

    vtkm::IdComponent index = 1;
    while (index < this->NumberOfColors && this->Pivots[index] < value)
      {
      index++;
      }

    if (index == this->NumberOfColors)
      {
      return this->Palette[index-1];
      }

    // TODO: cache the denominator
    float weight = ((value - this->Pivots[index-1])/
                    (this->Pivots[index] - this->Pivots[index-1]));

    return Lerp(this->Palette[index-1],this->Palette[index],weight);
  }

  VTKM_EXEC_CONT_EXPORT
  FPType operator()(const Color& color) const
  {
    // This doesn't have to be efficient. It is for testing purposes only.

    float weight;
    for (vtkm::IdComponent index = 0;index<this->NumberOfColors-1;index++)
      {
      if (this->ColorIsInInterval(color,index,weight))
        {
        return (1.-weight)*this->Pivots[index] + weight*this->Pivots[index+1];
        }
      }
    return this->Pivots[this->NumberOfColors-1];
  }

protected:
  VTKM_EXEC_CONT_EXPORT
  bool ColorIsInInterval(const Color& color,
                         vtkm::IdComponent interval,
                         float& weight) const
  {
    if (color == this->Palette[interval])
      {
      weight = 0.;
      return true;
      }

    typedef vtkm::Vec<float,4> SignedColor;
    SignedColor c0(this->Palette[interval]);
    SignedColor c1(this->Palette[interval+1]);
    SignedColor c(color);

    SignedColor c0_to_c(vtkm::Normal(c - c0));
    SignedColor c0_to_c1(vtkm::Normal(c1 - c0));

    SignedColor::ComponentType dot = c0_to_c.Dot(c0_to_c1);

    Color::ComponentType max(~0);

    if (vtkm::Abs(dot-1.) > 1./max)
      {
      weight = -1.;
      return false;
      }
    else
      {
      weight = (vtkm::Magnitude(c-c0)/vtkm::Magnitude(c0-c1));
      return true;
      }
  }
};

static
RuntimeColorTable make_ColorTable(ColorTable::Preset i,
                                  FPType min, FPType max)
{
  static std::vector<Color> palette;
  static std::vector<float> pivots;
  vtkm::Id numColors = 0;

  switch(i)
  {
    //1. ColorTable need to normalize the pivots after construction
    //based on the min & max
  case ColorTable::RUNTIME:
    //
    //Here be where we hack
    break;
  case ColorTable::COOLTOWARM:
    numColors = 3;
    palette.resize(numColors); pivots.resize(numColors);
    palette[0] = Color(59,76,192,255); pivots[0] = 0.0;
    palette[1] = Color(220,220,220,255); pivots[1] = .5;
    palette[2] = Color(180,4,38,255); pivots[2] = 1.;
    break;
  case ColorTable::BLACKBODY:
    numColors = 4;
    palette.resize(numColors); pivots.resize(numColors);
    palette[0] = Color(0,0,0,255); pivots[0] = 0.0;
    palette[1] = Color(230,0,0,255); pivots[1] = .4;
    palette[2] = Color(230,230,0,255); pivots[2] = .8;
    palette[3] = Color(255,255,255,255); pivots[3] = 1.;
    break;
  case ColorTable::BLUETOREDRAINBOW:
    numColors = 5;
    palette.resize(numColors); pivots.resize(numColors);
    palette[0] = Color(0,0,255,255); pivots[0] = 0.0;
    palette[1] = Color(0,255,255,255); pivots[1] = .25;
    palette[2] = Color(0,255,0,255); pivots[2] = 0.5;
    palette[3] = Color(255,255,0,255); pivots[3] = .75;
    palette[4] = Color(255,0,0,255); pivots[4] = 1.;
    break;
  case ColorTable::GRAYSCALE:
    numColors = 2;
    palette.resize(numColors); pivots.resize(numColors);
    palette[0] = Color(0,0,0,255); pivots[0] = 0.0;
    palette[1] = Color(255,255,255,255); pivots[1] = 1.;
    break;
  case ColorTable::GREENWHITELINEAR:
    numColors = 3;
    palette.resize(numColors); pivots.resize(numColors);
    palette[0] = Color(0,0,0,255); pivots[0] = 0.0;
    palette[1] = Color(15,138,54,255); pivots[1] = .5;
    palette[2] = Color(255,255,255,255); pivots[2] = 1.;
    break;
  }

  return RuntimeColorTable(min, max, palette, pivots);
}

static
RuntimeColorTable make_CustomColorTable(const uint8_t* rgba, const float* loc,
                                        size_t n, FPType cmin, FPType cmax) {
  std::vector<Color> palette(n);
  std::vector<float> pivots(n);
  for(size_t i=0; i < n; ++i) {
    palette[i] = Color(rgba[i*4+0], rgba[i*4+1], rgba[i*4+2], rgba[i*4+3]);
    pivots[i] = loc[i];
  }
  return RuntimeColorTable(cmin, cmax, palette, pivots);
}

#endif
