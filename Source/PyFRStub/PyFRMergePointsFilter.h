#ifndef PYFRMERGEPOINTSFILTER_H
#define PYFRMERGEPOINTSFILTER_H

class PyFRData;

struct PyFRMergePointsFilter
{
  void operator ()(PyFRData*,PyFRData*) const {}
};
#endif
