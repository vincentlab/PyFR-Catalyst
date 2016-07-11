#ifndef COLORTABLE_H
#define COLORTABLE_H

#define BOOST_SP_DISABLE_THREADS

#include <cassert>
#include <vector>
#include <stdio.h>

#include <vtkm/Types.h>
#include <vtkm/VecVariable.h>
#include <vtkm/VectorAnalysis.h>
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
  enum { MaxSize = 5 };

  enum Preset
  {
    COOLTOWARM,
    BLACKBODY,
    BLUETOREDRAINBOW,
    GRAYSCALE,
    GREENWHITELINEAR
  };

  VTKM_EXEC_CONT_EXPORT
  ColorTable()
  {
    this->NumberOfColors = MaxSize;
    PresetColorTable(BLUETOREDRAINBOW);
    this->Min = 0.;
    this->Max = 1.;
  }

  VTKM_EXEC_CONT_EXPORT
  ColorTable(const ColorTable& other)
  {
    this->Min = other.Min;
    this->Max = other.Max;
    this->NumberOfColors = other.NumberOfColors;
    for (unsigned i=0;i<MaxSize;i++)
      {
      this->Palette[i] = other.Palette[i];
      this->Pivots[i] = other.Pivots[i];
      }
  }

  VTKM_EXEC_CONT_EXPORT
  ColorTable& operator=(const ColorTable& other)
  {
    if (this != &other)
      {
      this->Min = other.Min;
      this->Max = other.Max;
      this->NumberOfColors = other.NumberOfColors;
      for (unsigned i=0;i<MaxSize;i++)
        {
        this->Palette[i] = other.Palette[i];
        this->Pivots[i] = other.Pivots[i];
        }
      }
    return *this;
  }

  VTKM_EXEC_CONT_EXPORT
  void PresetColorTable(Preset i)
  {
    switch(i)
      {
      case COOLTOWARM:
        SetNumberOfColors(3);
        SetPaletteColor(0,Color(59,76,192,255),0.);
        SetPaletteColor(1,Color(220,220,220,255),.5);
        SetPaletteColor(2,Color(180,4,38,255),1.);
        break;
      case BLACKBODY:
        SetNumberOfColors(4);
        SetPaletteColor(0,Color(0,0,0,255),0.);
        SetPaletteColor(1,Color(230,0,0,255),.4);
        SetPaletteColor(2,Color(230,230,0,255),.8);
        SetPaletteColor(3,Color(255,255,255,255),1.);
        break;
      case BLUETOREDRAINBOW:
        SetNumberOfColors(5);
        SetPaletteColor(0,Color(0,0,255,255),0.);
        SetPaletteColor(1,Color(0,255,255,255),.25);
        SetPaletteColor(2,Color(0,255,0,255),.5);
        SetPaletteColor(3,Color(255,255,0,255),.75);
        SetPaletteColor(4,Color(255,0,0,255),1.);
        break;
      case GRAYSCALE:
        SetNumberOfColors(2);
        SetPaletteColor(0,Color(0,0,0,255),0.);
        SetPaletteColor(1,Color(255,255,255,255),1.);
        break;
      case GREENWHITELINEAR:
        SetNumberOfColors(3);
        SetPaletteColor(0,Color(0,0,0,255),0.);
        SetPaletteColor(1,Color(15,138,54,255),.5);
        SetPaletteColor(2,Color(255,255,255,255),1.);
        break;
      }
  }

  VTKM_EXEC_CONT_EXPORT
  void SetRange(FPType min,FPType max)
  {
    float oldRange = this->Max - this->Min;
    float newRange = max - min;
    for (unsigned i=0;i<5;i++)
      {
      float tmp = this->Pivots[i];
      float normalizedPivot = (this->Pivots[i] - this->Min)/oldRange;
      this->Pivots[i] = min + normalizedPivot*newRange;
      }
    this->Min = min;
    this->Max = max;
  }

  VTKM_EXEC_CONT_EXPORT
  void SetNumberOfColors(vtkm::IdComponent nColors)
  {
    assert(nColors<=MaxSize);
    this->NumberOfColors = nColors;
  }

  VTKM_EXEC_CONT_EXPORT
  void SetPaletteColor(vtkm::IdComponent i,
                       const Color& color,
                       float normalizedPivot)
  {
    assert(i<MaxSize);
    this->Palette[i] = color;
    this->Pivots[i] = this->Min + normalizedPivot*(this->Max - this->Min);
  }

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

  FPType Min;
  FPType Max;
  vtkm::IdComponent NumberOfColors;
  vtkm::Vec<Color,MaxSize> Palette;
  vtkm::Vec<float,MaxSize> Pivots;
};

#endif
