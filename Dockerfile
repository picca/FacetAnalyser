FROM ubuntu:16.04 as builder

RUN apt-get update && apt-get install -y --no-install-recommends \
    git \
    ca-certificates `# essential for git over https` \
    build-essential \
    qt4-default libqt4-opengl-dev \
    libsm-dev libx11-dev libxt-dev libxext-dev `# needed for ITKVtkGlue` \
    curl

RUN apt-get update && apt-get install -y --no-install-recommends \
    libglew-dev libxt-dev libboost-all-dev mpi-default-dev libfontconfig1-dev

## new cmake essential to avoid not finding VTKConfig.cmake
RUN curl -s https://cmake.org/files/v3.11/cmake-3.11.4-Linux-x86_64.sh -o cmake.sh
RUN sh cmake.sh --prefix=/usr --exclude-subdir --skip-license

### PV with own VTK
RUN git clone -b v5.2.0 https://gitlab.kitware.com/paraview/paraview.git && \
    cd paraview && \
    git submodule update --init --recursive

RUN apt-get update && apt-get install -y --no-install-recommends \
libqt4-xmlpatterns qt4-dev-tools

RUN mkdir -p PV_build && \
    cd PV_build && \
    cmake \
    	  -DCMAKE_INSTALL_PREFIX=/opt/paraview/ \
	  -DCMAKE_BUILD_TYPE=Release \
	  -DBUILD_TESTING=OFF \
	  -DPARAVIEW_INSTALL_DEVELOPMENT_FILES=ON \
	  -DPARAVIEW_ENABLE_CATALYST=OFF \
	  ../paraview && \
    make -j"$(nproc)" && \
    make -j"$(nproc)" install

### ITK with VTK from PV for ITKVtkGlue
RUN git clone -b v4.12.2 https://itk.org/ITK.git

RUN mkdir -p ITK_build && \
    cd ITK_build && \
    cmake \
    	  -DCMAKE_INSTALL_PREFIX=/opt/itk/ \
	  -DVTK_DIR=/PV_build/VTK/ \
	  -DCMAKE_BUILD_TYPE=Release \
	  -DBUILD_SHARED_LIBS=ON \
	  -DBUILD_TESTING=OFF \
	  -DModule_ITKVtkGlue=ON \
	  -DModule_ITKReview=ON \
	  -DModule_LesionSizingToolkit=OFF \
	  ../ITK && \
    make -j"$(nproc)" && \
    make -j"$(nproc)" install

### FacetAnalyser
COPY code/ /code/

RUN cd PV_build/lib/ && \
    for i in *-pv*.so; do ln -s $i ${i%-pv*}.so; done && \
    for i in *-pv*.a; do ln -s $i ${i%-pv*}.a; done

RUN mkdir -p FacetAnalyser_build && \
    cd FacetAnalyser_build && \
    cmake \
    	  -DCMAKE_INSTALL_PREFIX=/opt/FacetAnalyser/ \
	  -DITK_DIR=/opt/itk/lib/cmake/ITK-4.12/ \
	  -DVTK_DIR=/PV_build/VTK/ \
	  -DParaView_DIR=/PV_build/ \
	  -DCMAKE_BUILD_TYPE=Release \
	  -DBUILD_PLUGIN=ON \
	  -DBUILD_EXAMPLE=ON \
	  -DBUILD_TESTING=OFF \
	  -DCMAKE_VERBOSE_MAKEFILE:BOOL=ON \
	  -DCMAKE_CXX_FLAGS="-I/PV_build/" \
	  -DCMAKE_LIBRARY_PATH=/PV_build/lib/ \
	  -DCMAKE_SHARED_LINKER_FLAGS=-L/PV_build/lib/ \
	  ../code/ && \
    make -j"$(nproc)" && \
    mkdir -p /opt/FacetAnalyser/bin/ && cp FacetAnalyserCLI /opt/FacetAnalyser/bin/


FROM ubuntu:16.04

COPY --from=builder /opt/paraview/ /opt/paraview/
COPY --from=builder /opt/itk/ /opt/itk/
COPY --from=builder /opt/FacetAnalyser/ /opt/FacetAnalyser/

ENV PATH="/opt/FacetAnalyser/bin/:${PATH}"
CMD ["/opt/FacetAnalyser/bin/FacetAnalyserCLI"]
