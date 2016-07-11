#ifndef PYFRCRINKLECLIPFILTER_H
#define PYFRCRINKLECLIPFILTER_H

#define BOOST_SP_DISABLE_THREADS

class PyFRData;

class PyFRCrinkleClipFilter
{
public:
  PyFRCrinkleClipFilter();
  virtual ~PyFRCrinkleClipFilter() {}

  void SetPlane(FPType,FPType,FPType,FPType,FPType,FPType);

  void operator ()(PyFRData*,PyFRData*) const;

  protected:
  FPType Origin[3];
  FPType Normal[3];
};

#endif
