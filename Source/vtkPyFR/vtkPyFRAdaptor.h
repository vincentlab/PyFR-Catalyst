#ifndef PYFRADAPTOR_HEADER
#define PYFRADAPTOR_HEADER

#ifdef __cplusplus
extern "C"
{
#endif
  void* CatalystInitialize(char* hostName, int port, char* outputfile, void* p);

  void CatalystFinalize(void* p);

  void CatalystCoProcess(double time, unsigned int timeStep, void* p, bool lastTimeStep=false);
#ifdef __cplusplus
}
#endif
#endif
