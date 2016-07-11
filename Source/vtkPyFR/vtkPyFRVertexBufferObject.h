#ifndef vtkPyFRVertexBufferObject_h
#define vtkPyFRVertexBufferObject_h

#include <vtkOpenGLVertexBufferObject.h>
#include "vtkPyFRContourData.h"


class VTKRENDERINGOPENGL2_EXPORT vtkPyFRVertexBufferObject :
  public vtkOpenGLVertexBufferObject
{
public:
  static vtkPyFRVertexBufferObject *New();
  vtkTypeMacro(vtkPyFRVertexBufferObject, vtkOpenGLVertexBufferObject)
  void PrintSelf(ostream& os, vtkIndent indent);

  //Create VBO's for the contour data
  void CreateVerticesVBO(vtkPyFRContourData* data, int index);
  void CreateNormalsVBO(vtkPyFRContourData* data, int index);
  void CreateColorsVBO(vtkPyFRContourData* data, int index);

protected:
  vtkPyFRVertexBufferObject();
  ~vtkPyFRVertexBufferObject();

private:
  vtkPyFRVertexBufferObject(const vtkPyFRVertexBufferObject&); // Not implemented
  void operator=(const vtkPyFRVertexBufferObject&); // Not implemented
};
#endif
