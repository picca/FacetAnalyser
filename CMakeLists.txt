
CMAKE_MINIMUM_REQUIRED(VERSION 3.8)

#Change PROJECT_NAME to the name of your project
PROJECT(FacetAnalyser)

#The following lines are required to use Dart
ENABLE_TESTING()
INCLUDE(CTest)

include(GNUInstallDirs)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_BINDIR}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR}")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR}")
set(BUILD_SHARED_LIBS ON)

FIND_PACKAGE(ITK REQUIRED)
INCLUDE(${ITK_USE_FILE})

INCLUDE_DIRECTORIES(${ITK_INCLUDE_DIRS} ${PROJECT_SOURCE_DIR}/third-party/VtkGlue/include/)
LINK_DIRECTORIES(${ITK_LIBRARY_DIRS})
LINK_LIBRARIES(${ITK_LIBRARIES})

#FIND_PACKAGE(VTK REQUIRED)
#INCLUDE(${VTK_USE_FILE})

FIND_PACKAGE(ParaView REQUIRED)
LINK_LIBRARIES(${VTK_LIBRARIES})

# build the plugin

set("_paraview_plugin_default_${CMAKE_PROJECT_NAME}" ON)
paraview_plugin_scan(
  PLUGIN_FILES      "${CMAKE_CURRENT_SOURCE_DIR}/Plugin/paraview.plugin"
  ENABLE_BY_DEFAULT ON
  PROVIDES_PLUGINS  plugins
  REQUIRES_MODULES  required_modules)

foreach (module IN LISTS required_modules)
  if (NOT TARGET "${module}")
    message(#FATAL_ERROR
      "Skipping example ${CMAKE_PROJECT_NAME}: Missing required module: "
      "${module}")
    return ()
  endif ()
endforeach ()

paraview_plugin_build(
  RUNTIME_DESTINATION "${CMAKE_INSTALL_BINDIR}"
  LIBRARY_DESTINATION "${CMAKE_INSTALL_LIBDIR}"
  LIBRARY_SUBDIRECTORY "${PARAVIEW_PLUGIN_SUBDIR}"
  PLUGINS ${plugins}
  )

# tests

add_test(
  NAME basicPluginTest01
  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/demos/
  COMMAND pvpython ${PROJECT_SOURCE_DIR}/scripts/pvsm2webgl.py -i 8faced-rhombic-dodecahedron_twinned_simp+def_PVplugin.pvsm -o ${PROJECT_SOURCE_DIR}/docs/demos/WebGL/8faced-rhombic-dodecahedron_twinned_simp+def_PVplugin.webgl -p ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR}/${PARAVIEW_PLUGIN_SUBDIR}/FacetAnalyser/FacetAnalyser.so
  )

set_property(
  TEST basicPluginTest01
  PROPERTY SKIP_REGULAR_EXPRESSION "AttributeError"
  )

add_test(
  NAME basicPluginTest02
  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/demos/
  COMMAND pvpython ${PROJECT_SOURCE_DIR}/scripts/pvsm2x3d.py -i 8faced-rhombic-dodecahedron_twinned_simp+def_PVplugin.pvsm -o ../docs/demos/X3D/8faced-rhombic-dodecahedron_twinned_simp+def_PVplugin.x3d -p ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR}/${PARAVIEW_PLUGIN_SUBDIR}/FacetAnalyser/FacetAnalyser.so
  ) # relative path as it is stored in X3D

set_property(
  TEST basicPluginTest02
  PROPERTY SKIP_REGULAR_EXPRESSION "AttributeError"
  )
