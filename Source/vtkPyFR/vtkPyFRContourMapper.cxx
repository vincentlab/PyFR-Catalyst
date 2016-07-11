#include "vtkPyFRContourMapper.h"

#include "vtkPyFRIndexBufferObject.h"
#include "vtkPyFRVertexBufferObject.h"

#include "vtkCommand.h"
#include "vtkExecutive.h"

#include "vtkOpenGLError.h"

#include "vtkDepthPeelingPass.h"
#include "vtkHardwareSelector.h"
#include "vtkOpenGLVertexArrayObject.h"
#include "vtkObjectFactory.h"
#include "vtkProperty.h"
#include "vtkRenderer.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkLight.h"
#include "vtkLightCollection.h"
#include "vtkMath.h"
#include "vtkPolyData.h"
#include "vtkRenderWindow.h"
#include "vtkShaderProgram.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include "vtkPyFRVertexBufferObject.h"
#include "vtkPyFRIndexBufferObject.h"

namespace
{
// helper to get the state of picking
int getPickState(vtkRenderer *ren)
{
  vtkHardwareSelector* selector = ren->GetSelector();
  if (selector)
    {
    return selector->GetCurrentPass();
    }

  if (ren->GetRenderWindow()->GetIsPicking())
    {
    return vtkHardwareSelector::ACTOR_PASS;
    }

  return vtkHardwareSelector::MIN_KNOWN_PASS - 1;
}
}


//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPyFRContourMapper)

//----------------------------------------------------------------------------
vtkPyFRContourMapper::vtkPyFRContourMapper():
  vtkOpenGLPolyDataMapper()
{
  this->ActiveContour = -1;
  this->ContourData = NULL;

  this->ColorVBO = vtkPyFRVertexBufferObject::New();
  // this->NormalVBO = vtkPyFRVertexBufferObject::New();

  //Important override the default VBO with our own custom VBO Implementation
  //Likewise the same needs
  this->VBO->Delete();
  this->VBO = vtkPyFRVertexBufferObject::New();

  //Important we need to override the IBO with our own IBO classes
  this->Tris.IBO->Delete();
  this->Tris.IBO = vtkPyFRIndexBufferObject::New();
}

//-----------------------------------------------------------------------------
vtkPyFRContourMapper::~vtkPyFRContourMapper()
{
  this->ColorVBO->Delete();
  this->ColorVBO = NULL;

  // this->NormalVBO->Delete();
  // this->NormalVBO = NULL;
}


//-----------------------------------------------------------------------------
void vtkPyFRContourMapper::ReleaseGraphicsResources(vtkWindow* win)
{
  this->ColorVBO->ReleaseGraphicsResources();
  // this->NormalVBO->ReleaseGraphicsResources();
  this->Superclass::ReleaseGraphicsResources(win);
}

//----------------------------------------------------------------------------
void vtkPyFRContourMapper::SetInputData(vtkPyFRContourData *input)
{
  this->SetInputDataInternal(0, input);
}

//----------------------------------------------------------------------------
// Specify the input data or filter.
vtkPyFRContourData *vtkPyFRContourMapper::GetInput()
{
  return vtkPyFRContourData::SafeDownCast(
    this->GetExecutive()->GetInputData(0, 0));
}

//----------------------------------------------------------------------------
int vtkPyFRContourMapper::FillInputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPyFRContourData");
  return 1;
}

//-----------------------------------------------------------------------------
void vtkPyFRContourMapper::RenderPiece(vtkRenderer* ren, vtkActor *actor)
{
  // Make sure that we have been properly initialized.
  if (ren->GetRenderWindow()->CheckAbortStatus())
    {
    return;
    }

  this->ContourData = this->GetInput();

  if (this->ContourData == NULL)
    {
    vtkErrorMacro(<< "No input!");
    return;
    }

  this->InvokeEvent(vtkCommand::StartEvent,NULL);
  if (!this->Static)
    {
    this->GetInputAlgorithm()->Update();
    }
  this->InvokeEvent(vtkCommand::EndEvent,NULL);

  // if there are no points then we are done
  if (this->ActiveContour == -1)
    {
    return;
    }

  this->RenderPieceStart(ren, actor);
  this->RenderPieceDraw(ren, actor);
  this->RenderEdges(ren,actor);
  this->RenderPieceFinish(ren, actor);
}

//-----------------------------------------------------------------------------
void vtkPyFRContourMapper::RenderPieceStart(vtkRenderer* ren, vtkActor *actor)
{
  // Set the PointSize and LineWidget
#if GL_ES_VERSION_2_0 != 1
  glPointSize(actor->GetProperty()->GetPointSize()); // not on ES2
#endif

  vtkHardwareSelector* selector = ren->GetSelector();
  int picking = getPickState(ren);
  if (this->LastSelectionState != picking)
    {
    this->SelectionStateChanged.Modified();
    this->LastSelectionState = picking;
    }

  if (selector && this->PopulateSelectionSettings)
    {
    selector->BeginRenderProp();
    // render points for point picking in a special way
    if (selector->GetFieldAssociation() == vtkDataObject::FIELD_ASSOCIATION_POINTS &&
        selector->GetCurrentPass() >= vtkHardwareSelector::ID_LOW24)
      {
#if GL_ES_VERSION_2_0 != 1
      glPointSize(4.0); //make verts large enough to be sure to overlap cell
#endif
      glEnable(GL_POLYGON_OFFSET_FILL);
      glPolygonOffset(0,2.0);  // supported on ES2/3/etc
      glDepthMask(GL_FALSE); //prevent verts from interfering with each other
      }
    if (selector->GetCurrentPass() == vtkHardwareSelector::COMPOSITE_INDEX_PASS)
      {
      selector->RenderCompositeIndex(1);
      }
    if (selector->GetCurrentPass() == vtkHardwareSelector::ID_LOW24 ||
        selector->GetCurrentPass() == vtkHardwareSelector::ID_MID24 ||
        selector->GetCurrentPass() == vtkHardwareSelector::ID_HIGH16)
      {
      selector->RenderAttributeId(0);
      }
    }

  this->TimeToDraw = 0.0;
  this->PrimitiveIDOffset = 0;

  // make sure the BOs are up to date
  this->UpdateBufferObjects(ren, actor);

  // Bind the OpenGL, this is shared between the different primitive/cell types.
  this->VBO->Bind();
  this->ColorVBO->Bind();
  // this->NormalVBO->Bind();

  this->LastBoundBO = NULL;
}

//-----------------------------------------------------------------------------
void vtkPyFRContourMapper::RenderPieceDraw(vtkRenderer* ren, vtkActor *actor)
{
  int representation = actor->GetProperty()->GetRepresentation();

  // render points for point picking in a special way
  // all cell types should be rendered as points
  vtkHardwareSelector* selector = ren->GetSelector();
  if (selector && this->PopulateSelectionSettings &&
      selector->GetFieldAssociation() == vtkDataObject::FIELD_ASSOCIATION_POINTS &&
      selector->GetCurrentPass() >= vtkHardwareSelector::ID_LOW24)
    {
    representation = VTK_POINTS;
    }

  vtkProperty *prop = actor->GetProperty();
  bool surface_offset =
    (this->GetResolveCoincidentTopology() || prop->GetEdgeVisibility())
    && prop->GetRepresentation() == VTK_SURFACE;

  if (surface_offset)
    {
    glEnable(GL_POLYGON_OFFSET_FILL);
    if ( this->GetResolveCoincidentTopology() == VTK_RESOLVE_SHIFT_ZBUFFER )
      {
      // do something rough is better than nothing
      double zRes = this->GetResolveCoincidentTopologyZShift(); // 0 is no shift 1 is big shift
      double f = zRes*4.0;
      glPolygonOffset(f + (prop->GetEdgeVisibility() ? 1.0 : 0.0),
        prop->GetEdgeVisibility() ? 1.0 : 0.0);  // supported on ES2/3/etc
      }
    else
      {
      double f, u;
      this->GetResolveCoincidentTopologyPolygonOffsetParameters(f,u);
      glPolygonOffset(f + (prop->GetEdgeVisibility() ? 1.0 : 0.0),
        u + (prop->GetEdgeVisibility() ? 1.0 : 0.0));  // supported on ES2/3/etc
      }
    }

  // draw polygons
  if (this->Tris.IBO->IndexCount)
    {
    // First we do the triangles, update the shader, set uniforms, etc.
    this->UpdateShaders(this->Tris, ren, actor);
    if (!this->HaveWideLines(ren,actor) && representation == VTK_WIREFRAME)
      {
      glLineWidth(actor->GetProperty()->GetLineWidth());
      }
    this->Tris.IBO->Bind();
    GLenum mode = (representation == VTK_POINTS) ? GL_POINTS :
      (representation == VTK_WIREFRAME) ? GL_LINES : GL_TRIANGLES;
    glDrawRangeElements(mode, 0,
                        static_cast<GLuint>(this->VBO->VertexCount - 1),
                        static_cast<GLuint>(this->ContourData->
                                            GetSizeOfContour(this->
                                                             ActiveContour)-1),
                      GL_UNSIGNED_INT,
                      reinterpret_cast<const GLvoid *>(NULL));
    this->Tris.IBO->Release();
    this->PrimitiveIDOffset += this->ContourData->GetSizeOfContour(this->ActiveContour)/3;
    }

  if (selector && (
        selector->GetCurrentPass() == vtkHardwareSelector::ID_LOW24 ||
        selector->GetCurrentPass() == vtkHardwareSelector::ID_MID24 ||
        selector->GetCurrentPass() == vtkHardwareSelector::ID_HIGH16))
    {
    selector->RenderAttributeId(this->PrimitiveIDOffset);
    }
}

//-----------------------------------------------------------------------------
void vtkPyFRContourMapper::RenderEdges(vtkRenderer* ren, vtkActor *actor)
{
}


//-----------------------------------------------------------------------------
void vtkPyFRContourMapper::RenderPieceFinish(vtkRenderer* ren,
  vtkActor *actor)
{
  if (this->LastBoundBO)
    {
    this->LastBoundBO->VAO->Release();
    }

  this->VBO->Release();
  this->ColorVBO->Release();

  vtkProperty *prop = actor->GetProperty();
  bool surface_offset =
    (this->GetResolveCoincidentTopology() || prop->GetEdgeVisibility())
    && prop->GetRepresentation() == VTK_SURFACE;
  if (surface_offset)
    {
    glDisable(GL_POLYGON_OFFSET_FILL);
    }

  // If the timer is not accurate enough, set it to a small
  // time so that it is not zero
  if (this->TimeToDraw == 0.0)
    {
    this->TimeToDraw = 0.0001;
    }

  this->UpdateProgress(1.0);
}

//-----------------------------------------------------------------------------
bool vtkPyFRContourMapper::GetNeedToRebuildShaders(
  vtkOpenGLHelper &cellBO, vtkRenderer* ren, vtkActor *actor)
{
  int lightComplexity = 0;

  // wacky backwards compatibility with old VTK lighting
  // soooo there are many factors that determine if a primative is lit or not.
  // three that mix in a complex way are representation POINT, Interpolation FLAT
  // and having normals or not.
  bool needLighting = false;
  bool haveNormals = false;
  bool isTrisOrStrips = (&cellBO == &this->Tris || &cellBO == &this->TriStrips);
  needLighting = (isTrisOrStrips ||
    (!isTrisOrStrips && actor->GetProperty()->GetInterpolation() != VTK_FLAT && haveNormals));

  // do we need lighting?
  if (actor->GetProperty()->GetLighting() && needLighting)
    {
    // consider the lighting complexity to determine which case applies
    // simple headlight, Light Kit, the whole feature set of VTK
    lightComplexity = 0;
    int numberOfLights = 0;
    vtkLightCollection *lc = ren->GetLights();
    vtkLight *light;

    vtkCollectionSimpleIterator sit;
    for(lc->InitTraversal(sit);
        (light = lc->GetNextLight(sit)); )
      {
      float status = light->GetSwitch();
      if (status > 0.0)
        {
        numberOfLights++;
        if (lightComplexity == 0)
          {
          lightComplexity = 1;
          }
        }

      if (lightComplexity == 1
          && (numberOfLights > 1
            || light->GetIntensity() != 1.0
            || light->GetLightType() != VTK_LIGHT_TYPE_HEADLIGHT))
        {
        lightComplexity = 2;
        }
      if (lightComplexity < 3
          && (light->GetPositional()))
        {
        lightComplexity = 3;
        break;
        }
      }
    }

  if (this->LastLightComplexity[&cellBO] != lightComplexity)
    {
    this->LightComplexityChanged[&cellBO].Modified();
    this->LastLightComplexity[&cellBO] = lightComplexity;
    }

  // check for prop keys
  vtkInformation *info = actor->GetPropertyKeys();
  int dp = (info && info->Has(vtkDepthPeelingPass::OpaqueZTextureUnit())) ? 1 : 0;

  if (this->LastDepthPeeling != dp)
    {
    this->DepthPeelingChanged.Modified();
    this->LastDepthPeeling = dp;
    }

  // has something changed that would require us to recreate the shader?
  // candidates are
  // property modified (representation interpolation and lighting)
  // input modified
  // light complexity changed
  if (cellBO.Program == 0 ||
      cellBO.ShaderSourceTime < this->GetMTime() ||
      cellBO.ShaderSourceTime < actor->GetMTime() ||
      cellBO.ShaderSourceTime < this->ContourData->GetMTime() ||
      cellBO.ShaderSourceTime < this->SelectionStateChanged ||
      cellBO.ShaderSourceTime < this->DepthPeelingChanged ||
      cellBO.ShaderSourceTime < this->LightComplexityChanged[&cellBO])
    {
    return true;
    }

  return false;
}

//----------------------------------------------------------------------------
int vtkPyFRContourMapper::ProcessRequest(vtkInformation* request,
                                      vtkInformationVector** inputVector,
                                      vtkInformationVector*)
{
  if(request->Has(vtkStreamingDemandDrivenPipeline::REQUEST_UPDATE_EXTENT()))
    {
    vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
    int currentPiece = this->NumberOfSubPieces * this->Piece;
    vtkStreamingDemandDrivenPipeline::SetUpdateExtent(
      inInfo,
      currentPiece,
      this->NumberOfSubPieces * this->NumberOfPieces,
      this->GhostLevel);
    }
  return 1;
}

//-------------------------------------------------------------------------
bool vtkPyFRContourMapper::GetNeedToRebuildBufferObjects(
  vtkRenderer *vtkNotUsed(ren), vtkActor *act)
{
  //this is only called from UpdateBufferObjects which is only called from
  //RenderPieceStart meaning that ContourData pointer will always be valid
  if (this->VBOBuildTime < this->GetMTime() ||
      this->VBOBuildTime < act->GetMTime() ||
      this->VBOBuildTime < this->ContourData->GetMTime() ||
      this->VBOBuildTime < this->SelectionStateChanged)
    {
    return true;
    }

  return false;
}

//-------------------------------------------------------------------------
void vtkPyFRContourMapper::BuildBufferObjects(vtkRenderer *ren, vtkActor *act)
{
  //Do not call MapScalars as that will cause bad things to happen
  //instead ContourData already has the rgba we need

  //we are always coloring by points
  this->HaveCellScalars = false;
  this->HaveCellNormals = false;

  // going to ignore the apple picking bug entirely
  this->HaveAppleBug = false;

  // rebuild the VBO and IBO if the data has changed
  if (this->VBOBuildTime < this->SelectionStateChanged ||
      this->VBOBuildTime < this->GetMTime() ||
      this->VBOBuildTime < act->GetMTime() ||
      this->VBOBuildTime < this->ContourData->GetMTime())
    {
    // Build the VBO's using our new vtkPyFRVertexBufferObject
    vtkPyFRVertexBufferObject* coordsVBO = dynamic_cast<vtkPyFRVertexBufferObject*>(this->VBO);
    coordsVBO->CreateVerticesVBO(this->ContourData, this->ActiveContour);
    this->ColorVBO->CreateColorsVBO(this->ContourData, this->ActiveContour);

    this->BuildIBO(ren, act, this->ContourData);
    }
}

//-------------------------------------------------------------------------
void vtkPyFRContourMapper::BuildIBO(
  vtkRenderer *ren,
  vtkActor *act,
  vtkPyFRContourData *contours)
{
  vtkPyFRIndexBufferObject* trisIBO = dynamic_cast<vtkPyFRIndexBufferObject*>(this->Tris.IBO);
  //uses the same logic points, since we share no verts in common
  trisIBO->CreateIndexBuffer(contours, this->ActiveContour);

}

//-------------------------------------------------------------------------
void vtkPyFRContourMapper::SetMapperShaderParameters(vtkOpenGLHelper &cellBO,
                                                      vtkRenderer* ren, vtkActor *actor)
{
  // Now to update the VAO too, if necessary.
  cellBO.Program->SetUniformi("PrimitiveIDOffset",
    this->PrimitiveIDOffset);

  if (cellBO.IBO->IndexCount && (this->VBOBuildTime > cellBO.AttributeUpdateTime ||
                                 cellBO.ShaderSourceTime > cellBO.AttributeUpdateTime))
    {
    //stride and offset are 0, since the attribute is tightly packed in
    //separate arrays
    const int offset = 0;
    const int stride = 0;

    cellBO.VAO->Bind();
    if (!cellBO.VAO->AddAttributeArray(cellBO.Program, this->VBO,
                                    "vertexMC", this->VBO->VertexOffset,
                                    this->VBO->Stride, VTK_FLOAT, 3, false))
      {
      vtkErrorMacro(<< "Error setting 'vertexMC' in shader VAO.");
      }

    // if (this->LastLightComplexity[&cellBO] > 0)
    //   {
    //   if (!cellBO.VAO->AddAttributeArray(cellBO.Program, this->NormalVBO,
    //                                      "normalMC", offset, stride, VTK_FLOAT, 3, false))
    //     {
    //     vtkErrorMacro(<< "Error setting 'normalMC' in shader VAO.");
    //     }
    //   }

    if (!this->DrawingEdges)
      {
      //stride is 0, since the attribute is tightly packed in the array
      if (!cellBO.VAO->AddAttributeArray(cellBO.Program, this->ColorVBO,
                                         "scalarColor", offset, stride,
                                         VTK_UNSIGNED_CHAR, 4, true))
        {
        vtkErrorMacro(<< "Error setting 'scalarColor' in shader VAO.");
        }
      }
    }

  // if depth peeling set the required uniforms
  vtkInformation *info = actor->GetPropertyKeys();
  if (info && info->Has(vtkDepthPeelingPass::OpaqueZTextureUnit()) &&
      info->Has(vtkDepthPeelingPass::TranslucentZTextureUnit()))
    {
    int otunit = info->Get(vtkDepthPeelingPass::OpaqueZTextureUnit());
    int ttunit = info->Get(vtkDepthPeelingPass::TranslucentZTextureUnit());
    cellBO.Program->SetUniformi("opaqueZTexture", otunit);
    cellBO.Program->SetUniformi("translucentZTexture", ttunit);

    int *renSize = info->Get(vtkDepthPeelingPass::DestinationSize());
    float screenSize[2];
    screenSize[0] = renSize[0];
    screenSize[1] = renSize[1];
    cellBO.Program->SetUniform2f("screenSize", screenSize);
    }


  if (this->GetNumberOfClippingPlanes())
    {
    // add all the clipping planes
    int numClipPlanes = this->GetNumberOfClippingPlanes();
    if (numClipPlanes > 6)
      {
      vtkErrorMacro(<< "OpenGL has a limit of 6 clipping planes");
      numClipPlanes = 6;
      }

    float planeEquations[6][4];
    for (int i = 0; i < numClipPlanes; i++)
      {
      double planeEquation[4];
      this->GetClippingPlaneInDataCoords(actor->GetMatrix(), i, planeEquation);
      planeEquations[i][0] = planeEquation[0];
      planeEquations[i][1] = planeEquation[1];
      planeEquations[i][2] = planeEquation[2];
      planeEquations[i][3] = planeEquation[3];
      }
    cellBO.Program->SetUniformi("numClipPlanes", numClipPlanes);
    cellBO.Program->SetUniform4fv("clipPlanes", 6, planeEquations);
    }

}

//----------------------------------------------------------------------------
void vtkPyFRContourMapper::ReplaceShaderColor(
  std::map<vtkShader::Type, vtkShader *> shaders,
  vtkRenderer *, vtkActor *actor)
{
  std::string VSSource = shaders[vtkShader::Vertex]->GetSource();
  std::string GSSource = shaders[vtkShader::Geometry]->GetSource();
  std::string FSSource = shaders[vtkShader::Fragment]->GetSource();

  // crate the material/color property declarations, and VS implementation
  // these are always defined
  std::string colorDec =
    "uniform float opacityUniform; // the fragment opacity\n"
    "uniform vec3 ambientColorUniform; // intensity weighted color\n"
    "uniform vec3 diffuseColorUniform; // intensity weighted color\n";
  // add some if we have a backface property
  if (actor->GetBackfaceProperty() && !this->DrawingEdges)
    {
    colorDec +=
      "uniform float opacityUniformBF; // the fragment opacity\n"
      "uniform vec3 ambientColorUniformBF; // intensity weighted color\n"
      "uniform vec3 diffuseColorUniformBF; // intensity weighted color\n";
    }
  // add more for specular
  if (this->LastLightComplexity[this->LastBoundBO])
    {
    colorDec +=
      "uniform vec3 specularColorUniform; // intensity weighted color\n"
      "uniform float specularPowerUniform;\n";
    if (actor->GetBackfaceProperty())
      {
      colorDec +=
        "uniform vec3 specularColorUniformBF; // intensity weighted color\n"
        "uniform float specularPowerUniformBF;\n";
      }
    }
  // add scalar vertex coloring
  if (!this->DrawingEdges)
    {
    colorDec += "varying vec4 vertexColorVSOutput;\n";
    vtkShaderProgram::Substitute(VSSource,"//VTK::Color::Dec",
                        "attribute vec4 scalarColor;\n"
                        "varying vec4 vertexColorVSOutput;");
    vtkShaderProgram::Substitute(VSSource,"//VTK::Color::Impl",
                        "vertexColorVSOutput =  scalarColor;");
    vtkShaderProgram::Substitute(GSSource,
      "//VTK::Color::Dec",
      "in vec4 vertexColorVSOutput[];\n"
      "out vec4 vertexColorGSOutput;");
    vtkShaderProgram::Substitute(GSSource,
      "//VTK::Color::Impl",
      "vertexColorGSOutput = vertexColorVSOutput[i];");
    }
  if (this->HaveCellScalars && !this->HavePickScalars && !this->DrawingEdges)
    {
    colorDec += "uniform samplerBuffer textureC;\n";
    }

  vtkShaderProgram::Substitute(FSSource,"//VTK::Color::Dec", colorDec);

  // now handle the more complex fragment shader implementation
  // the following are always defined variables.  We start
  // by assiging a default value from the uniform
  std::string colorImpl =
    "vec3 ambientColor;\n"
    "  vec3 diffuseColor;\n"
    "  float opacity;\n";
  if (this->LastLightComplexity[this->LastBoundBO])
    {
    colorImpl +=
      "  vec3 specularColor;\n"
      "  float specularPower;\n";
    }
  if (actor->GetBackfaceProperty())
    {
    if (this->LastLightComplexity[this->LastBoundBO])
      {
      colorImpl +=
        "  if (int(gl_FrontFacing) == 0) {\n"
        "    ambientColor = ambientColorUniformBF;\n"
        "    diffuseColor = diffuseColorUniformBF;\n"
        "    specularColor = specularColorUniformBF;\n"
        "    specularPower = specularPowerUniformBF;\n"
        "    opacity = opacityUniformBF; }\n"
        "  else {\n"
        "    ambientColor = ambientColorUniform;\n"
        "    diffuseColor = diffuseColorUniform;\n"
        "    specularColor = specularColorUniform;\n"
        "    specularPower = specularPowerUniform;\n"
        "    opacity = opacityUniform; }\n";
      }
    else
      {
      colorImpl +=
        "  if (int(gl_FrontFacing) == 0) {\n"
        "    ambientColor = ambientColorUniformBF;\n"
        "    diffuseColor = diffuseColorUniformBF;\n"
        "    opacity = opacityUniformBF; }\n"
        "  else {\n"
        "    ambientColor = ambientColorUniform;\n"
        "    diffuseColor = diffuseColorUniform;\n"
        "    opacity = opacityUniform; }\n";
      }
    }
  else
    {
    colorImpl +=
      "  ambientColor = ambientColorUniform;\n"
      "  diffuseColor = diffuseColorUniform;\n"
      "  opacity = opacityUniform;\n";
    if (this->LastLightComplexity[this->LastBoundBO])
      {
      colorImpl +=
        "  specularColor = specularColorUniform;\n"
        "  specularPower = specularPowerUniform;\n";
      }
    }

  // now handle scalar coloring
  if (!this->DrawingEdges)
    {
    if (this->ScalarMaterialMode == VTK_MATERIALMODE_AMBIENT ||
        (this->ScalarMaterialMode == VTK_MATERIALMODE_DEFAULT &&
          actor->GetProperty()->GetAmbient() > actor->GetProperty()->GetDiffuse()))
      {
      vtkShaderProgram::Substitute(FSSource,"//VTK::Color::Impl",
        colorImpl +
        "  ambientColor = vertexColorVSOutput.rgb;\n"
        "  opacity = opacity*vertexColorVSOutput.a;");
      }
    else if (this->ScalarMaterialMode == VTK_MATERIALMODE_DIFFUSE ||
        (this->ScalarMaterialMode == VTK_MATERIALMODE_DEFAULT &&
          actor->GetProperty()->GetAmbient() <= actor->GetProperty()->GetDiffuse()))
      {
      vtkShaderProgram::Substitute(FSSource,"//VTK::Color::Impl", colorImpl +
        "  diffuseColor = vertexColorVSOutput.rgb;\n"
        "  opacity = opacity*vertexColorVSOutput.a;");
      }
    else
      {
      vtkShaderProgram::Substitute(FSSource,"//VTK::Color::Impl", colorImpl +
        "  diffuseColor = vertexColorVSOutput.rgb;\n"
        "  ambientColor = vertexColorVSOutput.rgb;\n"
        "  opacity = opacity*vertexColorVSOutput.a;");
      }
    }
  else
    {
    // are we doing scalar coloring by texture?
    if (this->InterpolateScalarsBeforeMapping &&
        this->ColorCoordinates &&
        !this->DrawingEdges)
      {
      if (this->ScalarMaterialMode == VTK_MATERIALMODE_AMBIENT ||
          (this->ScalarMaterialMode == VTK_MATERIALMODE_DEFAULT &&
            actor->GetProperty()->GetAmbient() > actor->GetProperty()->GetDiffuse()))
        {
        vtkShaderProgram::Substitute(FSSource,
          "//VTK::Color::Impl", colorImpl +
          "  vec4 texColor = texture2D(texture1, tcoordVCVSOutput.st);\n"
          "  ambientColor = texColor.rgb;\n"
          "  opacity = opacity*texColor.a;");
        }
      else if (this->ScalarMaterialMode == VTK_MATERIALMODE_DIFFUSE ||
          (this->ScalarMaterialMode == VTK_MATERIALMODE_DEFAULT &&
           actor->GetProperty()->GetAmbient() <= actor->GetProperty()->GetDiffuse()))
        {
        vtkShaderProgram::Substitute(FSSource,
          "//VTK::Color::Impl", colorImpl +
          "  vec4 texColor = texture2D(texture1, tcoordVCVSOutput.st);\n"
          "  diffuseColor = texColor.rgb;\n"
          "  opacity = opacity*texColor.a;");
        }
      else
        {
        vtkShaderProgram::Substitute(FSSource,
          "//VTK::Color::Impl", colorImpl +
          "vec4 texColor = texture2D(texture1, tcoordVCVSOutput.st);\n"
          "  ambientColor = texColor.rgb;\n"
          "  diffuseColor = texColor.rgb;\n"
          "  opacity = opacity*texColor.a;");
        }
      }
    else
      {
      if (this->HaveCellScalars && !this->DrawingEdges)
        {
        if (this->ScalarMaterialMode == VTK_MATERIALMODE_AMBIENT ||
            (this->ScalarMaterialMode == VTK_MATERIALMODE_DEFAULT &&
              actor->GetProperty()->GetAmbient() > actor->GetProperty()->GetDiffuse()))
          {
          vtkShaderProgram::Substitute(FSSource,
            "//VTK::Color::Impl", colorImpl +
            "  vec4 texColor = texelFetchBuffer(textureC, gl_PrimitiveID + PrimitiveIDOffset);\n"
            "  ambientColor = texColor.rgb;\n"
            "  opacity = opacity*texColor.a;"
            );
          }
        else if (this->ScalarMaterialMode == VTK_MATERIALMODE_DIFFUSE ||
            (this->ScalarMaterialMode == VTK_MATERIALMODE_DEFAULT &&
             actor->GetProperty()->GetAmbient() <= actor->GetProperty()->GetDiffuse()))
          {
          vtkShaderProgram::Substitute(FSSource,
            "//VTK::Color::Impl", colorImpl +
           "  vec4 texColor = texelFetchBuffer(textureC, gl_PrimitiveID + PrimitiveIDOffset);\n"
            "  diffuseColor = texColor.rgb;\n"
            "  opacity = opacity*texColor.a;"
            //  "  diffuseColor = vec3((gl_PrimitiveID%256)/255.0,((gl_PrimitiveID/256)%256)/255.0,1.0);\n"
            );
          }
        else
          {
          vtkShaderProgram::Substitute(FSSource,
            "//VTK::Color::Impl", colorImpl +
            "vec4 texColor = texelFetchBuffer(textureC, gl_PrimitiveID + PrimitiveIDOffset);\n"
            "  ambientColor = texColor.rgb;\n"
            "  diffuseColor = texColor.rgb;\n"
            "  opacity = opacity*texColor.a;"
            );
          }
        }
      vtkShaderProgram::Substitute(FSSource,"//VTK::Color::Impl", colorImpl);
      }
    }

  shaders[vtkShader::Vertex]->SetSource(VSSource);
  shaders[vtkShader::Geometry]->SetSource(GSSource);
  shaders[vtkShader::Fragment]->SetSource(FSSource);
}

//----------------------------------------------------------------------------
void vtkPyFRContourMapper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
