#ifndef vtkPyFRIndexBufferObject_h
#define vtkPyFRIndexBufferObject_h

#include <vtkOpenGLIndexBufferObject.h>
#include "vtkPyFRContourData.h"

#include <vector>

class VTKRENDERINGOPENGL2_EXPORT vtkPyFRIndexBufferObject :
  public vtkOpenGLIndexBufferObject
{
public:
  static vtkPyFRIndexBufferObject *New();
  vtkTypeMacro(vtkPyFRIndexBufferObject, vtkOpenGLIndexBufferObject)
  void PrintSelf(ostream& os, vtkIndent indent);

  std::size_t CreateIndexBuffer(vtkPyFRContourData* data, int index);
  std::size_t CreateTriangleLineIndexBuffer(vtkPyFRContourData* data, int index);


protected:
  vtkPyFRIndexBufferObject();
  ~vtkPyFRIndexBufferObject();

private:
  vtkPyFRIndexBufferObject(const vtkPyFRIndexBufferObject&); // Not implemented
  void operator=(const vtkPyFRIndexBufferObject&); // Not implemented


  //hack to pre-compute the indexArray once, since it is just an
  //explicit array handle counting
  std::vector<unsigned int> IndexArray;
  bool UploadedIndices;

};
#endif
