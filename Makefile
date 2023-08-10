PROJECT = ska-pst-stat

DOCS_SOURCEDIR=./docs/src

# include OCI support
include .make/oci.mk

# include cpp make support
include .make/cpp.mk

# include k8s make support
include .make/k8s.mk

# include core make support
include .make/base.mk

# common pst makefile library
-include .pst/base.mk

# include your own private variables for custom deployment configuration
-include PrivateRules.mak

DEV_IMAGE	?=registry.gitlab.com/ska-telescope/pst/ska-pst-smrb/ska-pst-smrb-builder
DEV_TAG		?=0.10.0
PROCESSOR_COUNT=${nproc}
OCI_IMAGE_BUILD_CONTEXT=$(PWD)

PST_SMRB_OCI_REGISTRY		?=registry.gitlab.com/ska-telescope/pst/ska-pst-smrb
SMRB_RUNTIME_IMAGE			?=${PST_SMRB_OCI_REGISTRY}/ska-pst-smrb
STAT_BUILDER_IMAGE			?=${PST_SMRB_OCI_REGISTRY}/ska-pst-smrb-builder
STAT_RUNTIME_IMAGE			?=ubuntu:22.04
PST_SMRB_OCI_COMMON_TAG		?=${DEV_TAG}
OCI_BUILD_ADDITIONAL_ARGS	:=--build-arg SMRB_RUNTIME_IMAGE=${SMRB_RUNTIME_IMAGE}:${PST_SMRB_OCI_COMMON_TAG} --build-arg STAT_BUILDER_IMAGE=${STAT_BUILDER_IMAGE}:${PST_SMRB_OCI_COMMON_TAG} --build-arg STAT_RUNTIME_IMAGE=${STAT_RUNTIME_IMAGE}

# Extend pipeline machinery targets
.PHONY: docs-pre-build
docs-pre-build:
	@rm -rf docs/build/*
	apt-get update -y
	apt-get install -y doxygen
	pip3 install -r docs/requirements.txt

_VENV=.venv

local-apt-install-dependencies:
	apt-get update -y
	apt-get install -y `cat dependencies/apt.txt`
