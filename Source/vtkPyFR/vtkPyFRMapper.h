#ifndef vtkPyFRMapper_h
#define vtkPyFRMapper_h

#include "vtkRenderingCoreModule.h" // For export macro
#include "vtkMapper.h"
#include "vtkTexture.h" // used to include texture unit enum.

class vtkPyFRContourData;
class vtkRenderer;
class vtkRenderWindow;
class vtkPyFRMapperInternals;

class VTK_EXPORT vtkPyFRMapper : public vtkMapper
{
public:
  static vtkPyFRMapper *New();
  vtkTypeMacro(vtkPyFRMapper, vtkMapper);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Standard method for rendering a mapper. This method will be
  // called by the actor.
  void Render(vtkRenderer *ren, vtkActor *a);

  // Description:
  // Standard vtkProp method to get 3D bounds of a 3D prop
  double *GetBounds();
  void GetBounds(double bounds[6]) { this->Superclass::GetBounds( bounds ); };


  // Description:
  // Release the underlying resources associated with this mapper
  void ReleaseGraphicsResources(vtkWindow *);

protected:
  vtkPyFRMapper();
  ~vtkPyFRMapper();

  // Description:
  // Need to define the type of data handled by this mapper.
  virtual int FillInputPortInformation(int port, vtkInformation* info);

  // Description:
  // This is the build method for creating the internal polydata
  // mapper that do the actual work
  void BuildMappers();

  // Description:
  // Need to loop over the hierarchy to compute bounds
  void ComputeBounds();

  // Description:
  // Time stamp for computation of bounds.
  vtkTimeStamp BoundsMTime;

  // Description:
  // These are the internal polydata mapper that do the
  // rendering. We save then so that they can keep their
  // display lists.
  vtkPyFRMapperInternals *Internal;

  // Description:
  // Time stamp for when we need to update the
  // internal mappers
  vtkTimeStamp InternalMappersBuildTime;

private:
  vtkPyFRMapper(const vtkPyFRMapper&);  // Not implemented.
  void operator=(const vtkPyFRMapper&);  // Not implemented.
};

#endif
