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

  void CatalystImageResolution(void* p, const float color[3]);
#ifdef __cplusplus
}
#endif
#endif
