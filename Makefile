PROJECT = ska-pst-stat

DOCS_SOURCEDIR=./docs/src

PYTHON_SRC = python/src/
PYTHON_TEST_FILE = python/tests/
## Paths containing python to be formatted and linted
PYTHON_LINT_TARGET = python/src/ python/tests/
PYTHON_LINE_LENGTH = 110

# E203 and W503 conflict with black
PYTHON_SWITCHES_FOR_FLAKE8 = --extend-ignore=BLK,T --enable=DAR104 --ignore=E203,FS003,W503,N802 --max-complexity=10 \
    --rst-roles=py:attr,py:class,py:const,py:exc,py:func,py:meth,py:mod \
		--rst-directives deprecated,uml
PYTHON_SWITCHES_FOR_ISORT = --skip-glob="*/__init__.py" --py 310
PYTHON_SWITCHES_FOR_PYLINT = --disable=W,C,R
PYTHON_SWITCHES_FOR_AUTOFLAKE ?= --in-place --remove-unused-variables --remove-all-unused-imports --recursive --ignore-init-module-imports
PYTHON_SWITCHES_FOR_DOCFORMATTER ?= -r -i --black --style sphinx --wrap-summaries $(PYTHON_LINE_LENGTH) --wrap-descriptions $(PYTHON_LINE_LENGTH) --pre-summary-newline

PYTHON_VARS_AFTER_PYTEST = --cov-config=$(PWD)/.coveragerc

# Add this for typehints & static type checking
mypy:
	$(PYTHON_RUNNER) mypy --config-file mypy.ini $(PYTHON_LINT_TARGET)

flake8:
	$(PYTHON_RUNNER) flake8 --show-source --statistics $(PYTHON_SWITCHES_FOR_FLAKE8) $(PYTHON_LINT_TARGET)

python-post-format:
	$(PYTHON_RUNNER) autoflake $(PYTHON_SWITCHES_FOR_AUTOFLAKE) $(PYTHON_LINT_TARGET)
	$(PYTHON_RUNNER) docformatter $(PYTHON_SWITCHES_FOR_DOCFORMATTER) $(PYTHON_LINT_TARGET)

python-post-lint: mypy

.PHONY: python-post-format, notebook-format, notebook-post-format, python-post-lint, mypy, flake8

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

# include Python support
include .make/python.mk

# include your own private variables for custom deployment configuration
-include PrivateRules.mak

DEV_IMAGE	?=registry.gitlab.com/ska-telescope/pst/ska-pst-smrb/ska-pst-smrb-builder
DEV_TAG		?=0.10.5
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

_VENV=.venv

local-apt-install-dependencies:
	apt-get update -y
	apt-get install -y `cat dependencies/apt.txt`
	pip3 install -r docs/requirements.txt

