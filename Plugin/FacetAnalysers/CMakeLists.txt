# build the vtk modules and the paraview plugin

set(classes
  FacetAnalyser
  vtkGaussianSplatterExtended
  vtkMyHull
  )

vtk_module_add_module(FacetAnalysers
  FORCE_STATIC # Using FORCE_STATIC build the vtk module statically into the plugin library, to avoid confusion when loading
  CLASSES ${classes})

target_link_libraries(FacetAnalysers
  PRIVATE ${ITK_LIBRARIES}
  )

paraview_add_server_manager_xmls(
  XMLS  FacetAnalyser.xml)

# build and test the FacetAnalyserTest program

add_executable(FacetAnalyserTest
  FacetAnalyser.cxx
  FacetAnalyserTest.cxx
  vtkGaussianSplatterExtended.cxx
  vtkMyHull.cxx
  )
target_link_libraries(FacetAnalyserTest
  PRIVATE ${ITK_LIBRARIES} ${VTK_LIBRARIES}
  )

add_test(
  NAME basicTest01
  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/testing/
  COMMAND FacetAnalyserTest 8faced-rhombic-dodecahedron_twinned_simplified.vtk
  )

# build and test the FacetAnalyserCLI program

add_executable(FacetAnalyserCLI
  FacetAnalyser.cxx
  FacetAnalyserCLI.cxx
  vtkGaussianSplatterExtended.cxx
  vtkMyHull.cxx
  )
target_link_libraries(FacetAnalyserCLI
  PRIVATE ${ITK_LIBRARIES} ${VTK_LIBRARIES}
  )

add_test(
  NAME basicExampleTest01
  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/demos/
  COMMAND FacetAnalyserCLI 8faced-rhombic-dodecahedron_twinned_010_deformed_wss.vtp 51 15 0.2 0.001 2 0 8faced-rhombic-dodecahedron_twinned_010_deformed_wss_FA.vtp
  )
