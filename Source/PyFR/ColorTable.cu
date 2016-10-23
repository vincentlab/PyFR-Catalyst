
#include "ColorTable.h"

RuntimeColorTable::RuntimeColorTable(FPType cmin, FPType cmax,
  const std::vector<Color>& palette,
  const std::vector<float>& pivots)
{
  typedef ::vtkm::cont::DeviceAdapterTagCuda CudaTag;

  this->Min = cmin;
  this->Max = cmax;

  assert(palette.size() == pivots.size());
  this->NumberOfColors = static_cast<vtkm::Id>(palette.size());

  std::cout << "Min: " <<  this->Min << std::endl;
  std::cout << "Max: " <<  this->Max << std::endl;
  std::cout << "palette: " <<  palette.size() << std::endl;
  std::cout << "pivots: " <<  pivots.size() << std::endl;
  for(std::size_t i=0; i < palette.size(); ++i)
  {
    std::cout << "palette: " << palette[i] << std::endl;
    std::cout << "pivots: " << pivots[i] << std::endl;
  }

  typedef  vtkm::cont::ArrayHandle< Color > PaletteHandleType;
  typedef  vtkm::cont::ArrayHandle< float > PivotHandleType;

  PaletteHandleType ph = vtkm::cont::make_ArrayHandle(palette);
  PivotHandleType vh = vtkm::cont::make_ArrayHandle(pivots);

  this->Palette = ph.PrepareForInput(CudaTag());
  this->Pivots = vh.PrepareForInput(CudaTag());
  }