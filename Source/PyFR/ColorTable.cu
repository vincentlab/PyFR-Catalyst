
#include "ColorTable.h"



static ::thrust::system::cuda::vector< Color> PaletteVec;
static ::thrust::system::cuda::vector< float> PivotsVec;

RuntimeColorTable::RuntimeColorTable(FPType cmin, FPType cmax,
  const std::vector<Color>& palette,
  const std::vector<float>& pivots)
{
  typedef ::vtkm::cont::DeviceAdapterTagCuda CudaTag;

  this->Min = cmin;
  this->Max = cmax;

  assert(palette.size() == pivots.size());
  this->NumberOfColors = static_cast<vtkm::Id>(palette.size());

  std::vector<float> unnormalized_pivots(pivots.size());
  for(std::size_t i=0; i < pivots.size(); ++i)
  {
    unnormalized_pivots[i] =
              this->Min + pivots[i]*(this->Max - this->Min);
  }

  PaletteVec.resize(0);
  PivotsVec.resize(0);

  PaletteVec.assign(palette.begin(), palette.end());
  PivotsVec.assign(unnormalized_pivots.begin(), unnormalized_pivots.end());

  this->Palette = (&PaletteVec[0]).get();
  this->Pivots = (&PivotsVec[0]).get();

}


void RuntimeColorTable::ReleaseResources()
{
  PaletteVec.resize(0); PaletteVec.shrink_to_fit();
  PivotsVec.resize(0); PivotsVec.shrink_to_fit();
}