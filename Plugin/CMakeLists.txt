paraview_add_plugin(FacetAnalyser
  VERSION "1.0"
  MODULES FacetAnalysers
  MODULE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/FacetAnalysers/vtk.module")

target_include_directories(FacetAnalyser
  PRIVATE ${PROJECT_SOURCE_DIR}/third-party/VtkGlue/include/
  )
target_link_libraries(FacetAnalyser
  PRIVATE ${ITK_LIBRARIES}
  )
