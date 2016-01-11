/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPyFRIndexBufferObject.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkPyFRIndexBufferObject)

//-----------------------------------------------------------------------------
vtkPyFRIndexBufferObject::vtkPyFRIndexBufferObject():
  vtkOpenGLIndexBufferObject()
{
  this->IndexArray.reserve(2097152);
  //fill the array with a counting value
  for(unsigned int i=0; i < 2097152; ++i)
    { this->IndexArray.push_back(i); }
  this->UploadedIndices = false;
}

//-----------------------------------------------------------------------------
vtkPyFRIndexBufferObject::~vtkPyFRIndexBufferObject()
{

}

//-----------------------------------------------------------------------------
std::size_t vtkPyFRIndexBufferObject::CreateIndexBuffer(vtkPyFRContourData* data,
                                                             int index)
{
  const unsigned int numPoints =
      static_cast<unsigned int>(data->GetSizeOfContour(index) * 3);

  //extend the array if needed
  if(numPoints > this->IndexArray.size())
    {
    unsigned int start = static_cast<unsigned int>(this->IndexArray.size());

    this->IndexArray.reserve(numPoints);
    for(unsigned int i=start; i < numPoints; ++i)
      { this->IndexArray.push_back(i); }

    // this->UploadedIndices = false;
    }

  //this way we can build the vector once on the cpu, and re-use sections
  //of it as needed. Currently tuned to work best when we have less than
  //670k triangles per contour
  // if( this->IndexCount != numPoints )
    {
    this->Upload(&(this->IndexArray[0]),
                 numPoints,
                 vtkOpenGLIndexBufferObject::ElementArrayBuffer);
    // this->UploadedIndices = true;
    }

  return this->IndexCount = numPoints;
}

//-----------------------------------------------------------------------------
std::size_t vtkPyFRIndexBufferObject::CreateTriangleLineIndexBuffer(vtkPyFRContourData* data,
                                                             int index)
{
  const unsigned int numLines =
      static_cast<unsigned int>(data->GetSizeOfContour(index) * 6);

  //when we are drawing as lines we are going to upload a new ipo
  std::vector<unsigned int> lineIndexArray;
  lineIndexArray.reserve(numLines);

  for(unsigned int i=0; i < numLines; i+=2)
    {
    lineIndexArray.push_back(i);
    lineIndexArray.push_back(i+1);
    }


  this->Upload(lineIndexArray, vtkOpenGLIndexBufferObject::ElementArrayBuffer);

  return this->IndexCount = lineIndexArray.size();
}

//-----------------------------------------------------------------------------
void vtkPyFRIndexBufferObject::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
