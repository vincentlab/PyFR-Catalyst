#ifndef vtkPyFRContourMapper_h
#define vtkPyFRContourMapper_h

#include "vtkRenderingCoreModule.h" // For export macro
#include "vtkOpenGLPolyDataMapper.h"
#include "vtkTexture.h" // used to include texture unit enum.

class vtkPyFRContourData;
class vtkRenderer;
class vtkRenderWindow;
class vtkPyFRVertexBufferObject;
class vtkPyFRIndexBufferObject;

//We inherit from vtkOpenGLPolyDataMapper so that we can abuse all
//the existing shader code, even though we don't expect poly data input
class VTK_EXPORT vtkPyFRContourMapper : public vtkOpenGLPolyDataMapper
{
public:
  static vtkPyFRContourMapper *New();
  vtkTypeMacro(vtkPyFRContourMapper, vtkOpenGLPolyDataMapper);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Implemented by sub classes. Actual rendering is done here.
  virtual void RenderPiece(vtkRenderer *ren, vtkActor *act);
  virtual void RenderPieceStart(vtkRenderer *ren, vtkActor *act);
  virtual void RenderPieceDraw(vtkRenderer *ren, vtkActor *act);
  virtual void RenderPieceFinish(vtkRenderer *ren, vtkActor *act);
  virtual void RenderEdges(vtkRenderer *ren, vtkActor *act);

  virtual bool GetNeedToRebuildShaders(
    vtkOpenGLHelper &cellBO, vtkRenderer *ren, vtkActor *act);

  // Description:
  // Release any graphics resources that are being consumed by this mapper.
  // The parameter window could be used to determine which graphic
  // resources to release.
  void ReleaseGraphicsResources(vtkWindow *);

  // Description:
  // Specify the input data to map.
  void SetInputData(vtkPyFRContourData *in);
  vtkPyFRContourData *GetInput();

  // Description:
  // Specify the input contour to render.
  vtkSetMacro(ActiveContour, int);
  vtkGetMacro(ActiveContour, int);

  // Description:
  // If you want only a part of the data, specify by setting the piece.
  vtkSetMacro(Piece, int);
  vtkGetMacro(Piece, int);
  vtkSetMacro(NumberOfPieces, int);
  vtkGetMacro(NumberOfPieces, int);
  vtkSetMacro(NumberOfSubPieces, int);
  vtkGetMacro(NumberOfSubPieces, int);

  // Description:
  // Return bounding box (array of six doubles) of data expressed as
  // (xmin,xmax, ymin,ymax, zmin,zmax).
  // virtual double *GetBounds();
  // virtual void GetBounds(double bounds[6])
  //   { this->Superclass::GetBounds(bounds); }

  // Description:
  // see vtkAlgorithm for details
  virtual int ProcessRequest(vtkInformation*,
                             vtkInformationVector**,
                             vtkInformationVector*);
protected:
  vtkPyFRContourMapper();
  ~vtkPyFRContourMapper();

  // Description:
  // Does the VBO/IBO need to be rebuilt
  virtual bool GetNeedToRebuildBufferObjects(vtkRenderer *ren, vtkActor *act);

  // Description:
  // Build the VBO/IBO, called by UpdateBufferObjects
  virtual void BuildBufferObjects(vtkRenderer *ren, vtkActor *act);

  // Description:
  // Build the IBO, called by BuildBufferObjects
  virtual void BuildIBO(vtkRenderer *ren, vtkActor *act, vtkPyFRContourData *contour);

  // Description:
  // Set the shader parameteres related to the mapper/input data, called by UpdateShader
  virtual void SetMapperShaderParameters(vtkOpenGLHelper &cellBO, vtkRenderer *ren, vtkActor *act);

  // Description:
  // Perform string replacements on the shader templates, called from
  // ReplaceShaderValues. Only need to override the color one to handle
  // our multiple VBO's
  virtual void ReplaceShaderColor(
    std::map<vtkShader::Type, vtkShader *> shaders,
    vtkRenderer *ren, vtkActor *act);

  vtkPyFRContourData* ContourData;
  int ActiveContour;

  vtkPyFRVertexBufferObject *ColorVBO;
  // vtkPyFRVertexBufferObject *NormalVBO;

  virtual int FillInputPortInformation(int, vtkInformation*);

private:
  vtkPyFRContourMapper(const vtkPyFRContourMapper&);  // Not implemented.
  void operator=(const vtkPyFRContourMapper&);  // Not implemented.
};

#endif
