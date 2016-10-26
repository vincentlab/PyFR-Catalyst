#ifndef PYFRADAPTOR_HEADER
#define PYFRADAPTOR_HEADER

#ifdef __cplusplus
extern "C"
{
#endif
  void* CatalystInitialize(char* hostName, int port, char* outputfile, void* p);

  void CatalystFinalize(void* p);

  void CatalystCoProcess(double time, unsigned int timeStep, void* p, bool lastTimeStep=false);

  void CatalystCamera(void* p, const float eye[3], const float ref[3],
                      const float vup[3]);

  void CatalystBGColor(void* p, const float color[3]);

  void CatalystImageResolution(void* p, const uint32_t imgsz[2]);

  //By default all items use a coefficient of 0.5 and a power of 25
  //coefficient of 0 turns of specular highlights
  //power is 0 - 100
  void CatalystSpecularLighting(void* p, float coefficient, float power);

  void CatalystSetColorTable(void*, const uint8_t* rgba, const float* loc,
                             size_t n);
  void CatalystSetColorRange(void*, double low, double high);
#ifdef __cplusplus
}
#endif
#endif
