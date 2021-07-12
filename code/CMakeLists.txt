# Use the PLUGIN macro to create a plugin.
paraview_add_plugin(FacetAnalyser
  VERSION "1.0"
  SOURCES FacetAnalyser.cxx vtkGaussianSplatterExtended.cxx
  SERVER_MANAGER_XML FacetAnalyser.xml)

target_link_libraries(FacetAnalyser PRIVATE ${ITK_LIBRARIES})
install(TARGETS FacetAnalyser LIBRARY DESTINATION . )


IF(BUILD_TESTING)

  ## build test programs
  add_executable(FacetAnalyserTest
    FacetAnalyser.cxx
    FacetAnalyserTest.cxx
    vtkGaussianSplatterExtended.cxx
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
