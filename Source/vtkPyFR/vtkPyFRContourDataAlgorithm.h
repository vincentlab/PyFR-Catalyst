/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPolyDataAlgorithm.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPyFRAlgorithm - Superclass for algorithms that produce only
// vtkPyFRContourData as output
// .SECTION Description

#ifndef vtkPyFRContourDataAlgorithm_h
#define vtkPyFRContourDataAlgorithm_h

#include "vtkCommonExecutionModelModule.h" // For export macro
#include "vtkAlgorithm.h"
#include "vtkTypeTemplate.h" // For templated vtkObject API

class vtkDataSet;
class vtkPyFRContourData;

class VTK_EXPORT vtkPyFRContourDataAlgorithm : public vtkAlgorithm
{
public:
  static vtkPyFRContourDataAlgorithm *New();
  vtkTypeMacro(vtkPyFRContourDataAlgorithm,vtkAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Interface the algorithm to the Pipeline's passes.
  virtual int ProcessRequest(vtkInformation*,
                             vtkInformationVector**,
                             vtkInformationVector*);

  // Description:
  // Get the output data object for a port on this algorithm.
  vtkPyFRContourData* GetOutput();
  vtkPyFRContourData* GetOutput(int);
  virtual void SetOutput(vtkDataObject* d);

  // this method is not recommended for use, but lots of old style filters
  // use it
  vtkDataObject* GetInput();
  vtkDataObject* GetInput(int port);

  // Description:
  // Assign a data object as input. Note that this method does not
  // establish a pipeline connection. Use SetInputConnection() to
  // setup a pipeline connection.
  void SetInputData(vtkDataObject *);
  void SetInputData(int, vtkDataObject*);

  // Description:
  // Assign a data object as input. Note that this method does not
  // establish a pipeline connection. Use AddInputConnection() to
  // setup a pipeline connection.
  void AddInputData(vtkDataObject *);
  void AddInputData(int, vtkDataObject*);

protected:
  vtkPyFRContourDataAlgorithm();
  ~vtkPyFRContourDataAlgorithm();

  // convenience method
  virtual int RequestInformation(vtkInformation* request,
                                 vtkInformationVector** inputVector,
                                 vtkInformationVector* outputVector);

  // Description:
  // This is called by the superclass.
  // This is the method you should override.
  virtual int RequestData(vtkInformation* request,
                          vtkInformationVector** inputVector,
                          vtkInformationVector* outputVector);

  // Description:
  // Produce empty output of the proper type for RequestData to fill in.
  virtual int RequestDataObject(vtkInformation* request,
                                vtkInformationVector** inputVector,
                                vtkInformationVector* outputVector);

  // Description:
  // This is called by the superclass.
  // This is the method you should override.
  virtual int RequestUpdateExtent(vtkInformation*,
                                  vtkInformationVector**,
                                  vtkInformationVector*);

  // Overridden to say that we take in and produce vtkPyFRContourData objects
  virtual int FillOutputPortInformation(int port, vtkInformation* info);
  virtual int FillInputPortInformation(int port, vtkInformation* info);

private:
  vtkPyFRContourDataAlgorithm(const vtkPyFRContourDataAlgorithm&);  // Not implemented.
  void operator=(const vtkPyFRContourDataAlgorithm&);  // Not implemented.
};

#endif
