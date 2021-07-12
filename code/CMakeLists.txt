# Use the PLUGIN macro to create a plugin.
paraview_add_plugin(FacetAnalyser
  VERSION "1.0"
  SOURCES FacetAnalyser.cxx vtkGaussianSplatterExtended.cxx
  SERVER_MANAGER_XML FacetAnalyser.xml)

TARGET_LINK_LIBRARIES(FacetAnalyser PRIVATE ${ITK_LIBRARIES})
INSTALL(TARGETS FacetAnalyser LIBRARY DESTINATION . )
