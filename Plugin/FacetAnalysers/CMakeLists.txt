set(classes
  FacetAnalyser
  vtkGaussianSplatterExtended
  vtkMyHull
  )

vtk_module_add_module(FacetAnalysers
  FORCE_STATIC # Using FORCE_STATIC build the vtk module statically into the plugin library, to avoid confusion when loading
  CLASSES ${classes})

target_include_directories(FacetAnalysers
  PRIVATE ${PROJECT_SOURCE_DIR}/third-party/VtkGlue/include/
  )
target_link_libraries(FacetAnalysers
  PRIVATE ${ITK_LIBRARIES}
  )

paraview_add_server_manager_xmls(
  XMLS  FacetAnalyser.xml)

IF(BUILD_TESTING)

  ## build test programs
  add_executable(FacetAnalyserTest
    FacetAnalyser.cxx
    FacetAnalyserTest.cxx
    vtkGaussianSplatterExtended.cxx
    vtkMyHull.cxx
    )
  target_include_directories(FacetAnalyserTest
    PRIVATE ${PROJECT_SOURCE_DIR}/third-party/VtkGlue/include/
    )
  target_link_libraries(FacetAnalyserTest
    PRIVATE ${ITK_LIBRARIES} ${VTK_LIBRARIES}
    )

  ## run tests
  add_test(
    NAME basicTest01
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/testing/
    COMMAND FacetAnalyserTest 8faced-rhombic-dodecahedron_twinned_simplified.vtk
    )

ENDIF(BUILD_TESTING)

IF(BUILD_EXAMPLE)

  add_executable(FacetAnalyserCLI
    FacetAnalyser.cxx
    FacetAnalyserCLI.cxx
    vtkGaussianSplatterExtended.cxx
    vtkMyHull.cxx
    )
  target_include_directories(FacetAnalyserCLI
    PRIVATE ${PROJECT_SOURCE_DIR}/third-party/VtkGlue/include/
    )
  target_link_libraries(FacetAnalyserCLI
    PRIVATE ${ITK_LIBRARIES} ${VTK_LIBRARIES}
    )

  IF(BUILD_TESTING)

    add_test(
      NAME basicExampleTest01
      WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/demos/
      COMMAND FacetAnalyserCLI 8faced-rhombic-dodecahedron_twinned_010_deformed_wss.vtp 51 15 0.2 0.001 2 8faced-rhombic-dodecahedron_twinned_010_deformed_wss_FA.vtp
      )

  ENDIF(BUILD_TESTING)

ENDIF(BUILD_EXAMPLE)
