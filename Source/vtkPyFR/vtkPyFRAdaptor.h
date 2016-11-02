#ifndef PYFRADAPTOR_HEADER
#define PYFRADAPTOR_HEADER

#ifdef __cplusplus
extern "C"
{
#endif

  //pipeline 1=contour
  //pipeline 2=slice
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

  void CatalystFilenamePrefix(void* p, const char* pfix);
  void CatalystSetColorTable(void*, const uint8_t* rgba, const float* loc,
                             size_t n, int pipeline);
  void CatalystSetColorRange(void*, double low, double high, int pipeline);

  //Fields:
  //0 = "density";
  //1 = "pressure";
  //2 = "velocity_u";
  //3 = "velocity_v";
  //4 = "velocity_w";
  //5 = "velocity_magnitude";
  //6 = "density_gradient_magnitude";
  //7 = "pressure_gradient_magnitude";
  //8 = "velocity_gradient_magnitude";
  //9 = "velocity_qcriterion";
  //
  void CatalystSetFieldToContourBy(int field);

  //Specify the location and number of slice planes
  void CatalystSetSlicePlanes(float origin[3], float normal[3],
                              int number, double spacing);

  //Specify the location of the two clip planes for the contour pipeline
  void CatalystSetClipPlanes(float origin1[3], float normal1[3],
                             float origin2[3], float normal2[3]);

  //Depending on what mode we initialized catalyst in, this will change
  //field to color the contour or slice by
  void CatalystSetFieldToColorBy(int field, int pipeline);


#ifdef __cplusplus
}
#endif
#endif
