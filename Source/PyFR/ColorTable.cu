
#include "ColorTable.h"

static ::thrust::system::cuda::vector< Color> PaletteVec[2];
static ::thrust::system::cuda::vector< float> PivotsVec[2];
static std::vector<Color> RuntimePalette[2];
static std::vector<float> RuntimePivots[2];

RuntimeColorTable::RuntimeColorTable(int pipeline, FPType cmin, FPType cmax,
  const std::vector<Color>& palette, const std::vector<float>& pivots)
{
  typedef ::vtkm::cont::DeviceAdapterTagCuda CudaTag;

  this->WhichPipeline = pipeline;
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

  int p = pipeline -1;
  PaletteVec[p].resize(0);
  PivotsVec[p].resize(0);

  PaletteVec[p].assign(palette.begin(), palette.end());
  PivotsVec[p].assign(unnormalized_pivots.begin(), unnormalized_pivots.end());

  this->Palette = (&PaletteVec[p][0]).get();
  this->Pivots = (&PivotsVec[p][0]).get();

  std::cout << "RuntimeColorTable being constructed. " << std::endl;
  std::cout << "RuntimeColorTable->Min: " << this->Min << std::endl;
  std::cout << "RuntimeColorTable->Max: " << this->Max << std::endl;
  std::cout << "RuntimeColorTable->NumberOfColors: " << this->NumberOfColors << std::endl;
  std::cout << "RuntimeColorTable->WhichPipeline: " << this->WhichPipeline << std::endl;
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
  PaletteVec[0].resize(0); PaletteVec[0].shrink_to_fit();
  PivotsVec[0].resize(0); PivotsVec[0].shrink_to_fit();
  PaletteVec[1].resize(0); PaletteVec[1].shrink_to_fit();
  PivotsVec[1].resize(0); PivotsVec[1].shrink_to_fit();
}

void fetchRuntimeVectors(int pipeline,
                         std::vector<Color>& palette,
                         std::vector<float>& pivots)
{
  int p = pipeline - 1;
  palette = std::vector<Color>(RuntimePalette[p].begin(), RuntimePalette[p].end());
  pivots = std::vector<float>(RuntimePivots[p].begin(), RuntimePivots[p].end());
}

void fillRuntimeVectors(int pipeline, const uint8_t* rgba, const float* loc, size_t n)
{
  int p = pipeline - 1;
  RuntimePalette[p].resize(n);
  RuntimePivots[p].resize(n);

  for(size_t i=0; i < n; ++i) {
    RuntimePalette[p][i] = Color(rgba[i*4+0], rgba[i*4+1], rgba[i*4+2], rgba[i*4+3]);
    RuntimePivots[p][i] = loc[i];
  }
}


