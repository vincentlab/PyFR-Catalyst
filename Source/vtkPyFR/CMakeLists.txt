set(vtkPyFR_SRCS
  vtkPyFRContourData.cxx
  vtkPyFRContourDataAlgorithm.cxx
  vtkPyFRContourDataConverter.cxx
  vtkPyFRContourFilter.cxx
  vtkPyFRContourMapper.cxx
  vtkPyFRCrinkleClipFilter.cxx
  vtkPyFRData.cxx
  vtkPyFRDataAlgorithm.cxx
  vtkPyFRDataConverter.cxx
  vtkPyFRGradientFilter.cxx
  vtkPyFRIndexBufferObject.cxx
  vtkPyFRMapper.cxx
  vtkPyFRMergePointsFilter.cxx
  vtkPyFRParallelSliceFilter.cxx
  vtkPyFRVertexBufferObject.cxx
  vtkXMLPyFRContourDataWriter.cxx
  vtkXMLPyFRDataWriter.cxx
  )

set (vtkPyFR_catalyst_SRCS
  vtkPyFRPipeline.cxx
  vtkPyFRAdaptor.cxx)

set( vtkPyFRLibs )

if (BuildServerLibs)
  foreach (fpType IN LISTS fpList)
    set (pyfrLib "pyfr_${libSuffix_${fpType}}")
    set (pluginLib "pyfr_plugin_${libSuffix_${fpType}}")
    set (catalystLib "pyfr_catalyst_${libSuffix_${fpType}}")
    set( fp_cxx_flags "-DFPType=${fpType} -DDATA_DIR=${CMAKE_INSTALL_PREFIX}/data" )
    if (${fpType} STREQUAL float)
      set( fp_cxx_flags "${fp_cxx_flags} -DPYFR_SINGLE" )
    endif()

    add_paraview_plugin(${pluginLib} "1.0" SERVER_MANAGER_XML PyFR.xml SERVER_MANAGER_SOURCES ${vtkPyFR_SRCS})
    set_target_properties(${pluginLib} PROPERTIES COMPILE_FLAGS ${fp_cxx_flags})

    target_include_directories(${pluginLib} PUBLIC ${PROJECT_SOURCE_DIR}/Source/PyFR)
    target_link_libraries(${pluginLib} PRIVATE vtkPVCatalyst vtkPVVTKExtensionsDefault vtkPVClientServerCoreCore ${pyfrLib} ${CUDA_LIBRARIES} ${MPI_LIBRARIES})
    if(MPI_COMPILE_FLAGS)
      set_target_properties(${pluginLib} PROPERTIES
	COMPILE_FLAGS "${MPI_COMPILE_FLAGS}")
    endif()
    if(MPI_LINK_FLAGS)
      set_target_properties(${pluginLib} PROPERTIES
	LINK_FLAGS "${MPI_LINK_FLAGS}")
    endif()
    list( APPEND vtkPyFRLibs ${pluginLib} )

    add_library(${catalystLib} SHARED ${vtkPyFR_catalyst_SRCS})
    set_target_properties(${catalystLib} PROPERTIES COMPILE_FLAGS ${fp_cxx_flags})
    target_include_directories(${catalystLib} PUBLIC ${PROJECT_SOURCE_DIR}/Source/PyFR)
    target_link_libraries(${catalystLib} PRIVATE vtkPVCatalyst vtkPVVTKExtensionsDefault vtkPVClientServerCoreCore ${VTK_LIBRARIES} ${pluginLib} ${pyfrLib} ${CUDA_LIBRARIES} ${MPI_LIBRARIES})
    if(MPI_COMPILE_FLAGS)
      set_target_properties(${catalystLib} PROPERTIES
	COMPILE_FLAGS "${MPI_COMPILE_FLAGS}")
    endif()
    if(MPI_LINK_FLAGS)
      set_target_properties(${catalystLib} PROPERTIES
	LINK_FLAGS "${MPI_LINK_FLAGS}")
    endif()
    list( APPEND vtkPyFRLibs ${catalystLib} )
  endforeach()
endif()

if (BuildClientLibs)
  add_paraview_plugin(pyfr_plugin "1.0" SERVER_MANAGER_XML PyFR.xml SERVER_MANAGER_SOURCES ${vtkPyFR_SRCS})
  set_target_properties(pyfr_plugin PROPERTIES COMPILE_FLAGS "-DFPType=double")
  target_include_directories(pyfr_plugin PUBLIC ${PROJECT_SOURCE_DIR}/Source/PyFRStub)
  target_link_libraries(pyfr_plugin PRIVATE vtkPVCatalyst vtkPVVTKExtensionsDefault vtkPVClientServerCoreCore)
  list( APPEND vtkPyFRLibs pyfr_plugin )
endif()

install(TARGETS ${vtkPyFRLibs} DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)
