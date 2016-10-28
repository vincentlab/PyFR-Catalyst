
#include "ColorTable.h"

static ::thrust::system::cuda::vector< Color> PaletteVec;
static ::thrust::system::cuda::vector< float> PivotsVec;
static std::vector<Color> RuntimePalette;
static std::vector<float> RuntimePivots;

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

  std::cout << "RuntimeColorTable being constructed. " << std::endl;
  std::cout << "RuntimeColorTable->Min: " << this->Min << std::endl;
  std::cout << "RuntimeColorTable->Max: " << this->Max << std::endl;
  std::cout << "RuntimeColorTable->NumberOfColors: " << this->NumberOfColors << std::endl;
  for(std::size_t i=0; i < pivots.size(); ++i)
  {
    std::cout <<  "RuntimeColorTable pivots["<<i<<"] = " << unnormalized_pivots[i] << std::endl;
    std::cout <<  "RuntimeColorTable rgba["<<i<<"] = ("
              << (int)palette[i][0] << ", "
              << (int)palette[i][1] << ", "
              << (int)palette[i][2] << ", "
              << (int)palette[i][3]
              << ")"<<std::endl;
  }

}

void RuntimeColorTable::ReleaseResources()
{
  PaletteVec.resize(0); PaletteVec.shrink_to_fit();
  PivotsVec.resize(0); PivotsVec.shrink_to_fit();
}

void fetchRuntimeVectors(std::vector<Color>& palette,
                         std::vector<float>& pivots)
{
  palette = std::vector<Color>(RuntimePalette.begin(), RuntimePalette.end());
  pivots = std::vector<float>(RuntimePivots.begin(), RuntimePivots.end());
}

void fillRuntimeVectors(const uint8_t* rgba, const float* loc, size_t n)
{
  RuntimePalette.resize(n);
  RuntimePivots.resize(n);

  for(size_t i=0; i < n; ++i) {
    RuntimePalette[i] = Color(rgba[i*4+0], rgba[i*4+1], rgba[i*4+2], rgba[i*4+3]);
    RuntimePivots[i] = loc[i];
  }
}


