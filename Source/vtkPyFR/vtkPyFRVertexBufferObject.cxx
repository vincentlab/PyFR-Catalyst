/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPyFRVertexBufferObject.h"
#include "vtkObjectFactory.h"

#include "PyFRContourData.h"

vtkStandardNewMacro(vtkPyFRVertexBufferObject)

//-----------------------------------------------------------------------------
vtkPyFRVertexBufferObject::vtkPyFRVertexBufferObject():
  vtkOpenGLVertexBufferObject()
{
}

//-----------------------------------------------------------------------------
vtkPyFRVertexBufferObject::~vtkPyFRVertexBufferObject()
{

}

//-----------------------------------------------------------------------------
void vtkPyFRVertexBufferObject::CreateVerticesVBO(vtkPyFRContourData* data,
                                                  int index)
{
  //Set our type to array buffer by default
  this->SetType(vtkOpenGLBufferObject::ArrayBuffer);

  //setup some minor info for this vbo, only the vertice vbo is queried
  //about the state of these member variables
  int blockSize = 3;
  this->VertexOffset = 0;
  this->NormalOffset = 0;
  this->TCoordOffset = 0;
  this->TCoordComponents = 0;
  this->ColorComponents = 0;
  this->ColorOffset = 0;
  this->Stride = sizeof(float) * blockSize;
  this->VertexCount = data->GetSizeOfContour(index);

  //use vtkm to transfer the cuda allocated memory over to opengl
  //without having to transfer back to main memory
  transfer::coords(data->GetData(),index,this->GetHandleRef());
}

//-----------------------------------------------------------------------------
void vtkPyFRVertexBufferObject::CreateNormalsVBO(vtkPyFRContourData* data,
                                                  int index)
{
  //Set our type to array buffer by default
  this->SetType(vtkOpenGLBufferObject::ArrayBuffer);


  this->VertexCount = data->GetSizeOfContour(index);

  //use vtkm to transfer the cuda allocated memory over to opengl
  //without having to transfer back to main memory
  transfer::normals(data->GetData(),index,this->GetHandleRef());
}

//-----------------------------------------------------------------------------
void vtkPyFRVertexBufferObject::CreateColorsVBO(vtkPyFRContourData* data,
                                                int index)
{
  //Set our type to array buffer by default
  this->SetType(vtkOpenGLBufferObject::ArrayBuffer);

  this->VertexCount = data->GetSizeOfContour(index);

  //use vtkm to transfer the cuda allocated memory over to opengl
  //without having to transfer back to main memory
  transfer::colors(data->GetData(),index,this->GetHandleRef());
}

//-----------------------------------------------------------------------------
void vtkPyFRVertexBufferObject::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
