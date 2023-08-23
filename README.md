# ska-pst-stat

This project provides the C++ library and applications for the STAT component of the Pulsar Timing instrument for SKA Mid and SKA Low.

## Documentation

[![Documentation Status](https://readthedocs.org/projects/ska-telescope-ska-pst-stat/badge/?version=latest)](https://developer.skao.int/projects/ska-pst-stat/en/latest/)

The documentation for this project, including the package description, Architecture description and the API modules can be found at SKA developer portal:  [https://developer.skao.int/projects/ska-pst-stat/en/latest/](https://developer.skao.int/projects/ska-pst-stat/en/latest/)

## Build Instructions

Firstly clone this repo and submodules to your local file system

    git clone --recursive git@gitlab.com:ska-telescope/pst/ska-pst-stat.git

then change to the newly cloned directory and create the build/ sub-directory

    cd ska-pst-stat
    mkdir build

To simulate the build that will be performed in the Gitlab CI, a C++ builder image that has been extended from the ska-cicd-cpp-build-base [C++ building image](https://github.com/ska-telescope/cpp_build_base) is used. This image includes the required OS package and other custom software dependencies. The current version of this image is defined in .gitlab-ci.yml; e.g.

    grep SKA_CPP_DOCKER_BUILDER_IMAGE .gitlab-ci.yml

After verifying the current builder image version, pull it with a command like the following

    docker pull registry.gitlab.com/ska-telescope/pst/ska-pst-smrb/ska-pst-smrb-builder:0.10.2

Now launch this builder image as a container. Note the current working directory will be mounted into the container as /mnt/ska-pst-stat.

    make local-dev-env

After the builder container is launched, install any residual dependencies with

    make local-apt-install-dependencies

The STAT library and applications can now be built using the standard makefile templates provided by CICD infrastructure.

### Debug Build

The debug build will use the Cmake build arguments `-DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_FLAGS="-coverage" -DCMAKE_EXE_LINKER_FLAGS="-coverage"`. For debug purposes, the `#define DEBUG` will be defined for the software to enable any debug features.

    make local-cpp-build-debug

### Release Build

The release build will use the Cmake build arguments `-DCMAKE_BUILD_TYPE=Release` which ensures `#define DEBUG` is not defined. This build should be used for all deployments.

    make local-cpp-build-release

### Linting Build

This build target compiles the library and applications with the flags required for linting and static analysis.

    make local-cpp-ci-simulation-lint

During the compilation, the build generates `compile_commands.json` file which is used in the linting and static analysis tools: clang-tidy, cppcheck and IWYU.

### Documentation Build

API documentation for the library is genereated with Doxygen, which is then converted into ReadTheDocs format by Sphinx, Breathe and Exhale. The documentation is built via 

    make docs-build html

## Building Docker Image

To build an OCI Docker image for testing the applications:
