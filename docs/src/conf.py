# Configuration file for the Sphinx documentation builder.
#
# This file only contains a selection of the most common options. For a full
# list see the documentation:
# https://www.sphinx-doc.org/en/master/usage/configuration.html

# -- Path setup --------------------------------------------------------------

# If extensions (or modules to document with autodoc) are in another directory,
# add these directories to sys.path here. If the directory is relative to the
# documentation root, use os.path.abspath to make it absolute, like shown here.
#
# import os
# import sys
# sys.path.insert(0, os.path.abspath('.'))


import os
import sys
import subprocess
import shutil
import textwrap

def setup(app):
    app.add_css_file('css/custom.css')

# -- Doxygen Generate --------------------------------------------------------

def configureDoxyfile(input_dir: str, output_dir: str):
    with open('../Doxyfile', 'r') as file:
        file_data = file.read()

    file_data = file_data.replace('@DOXYGEN_INPUT_DIR@', input_dir)
    file_data = file_data.replace('@DOXYGEN_OUTPUT_DIR@', output_dir)

    with open('../Doxyfile', 'w') as file:
        file.write(file_data)

# -- Project information -----------------------------------------------------

project = 'SKA PST STAT'
copyright = '2023 Square Kilometre Array Observatory'
author = 'PST Team'

# The full version, including alpha/beta/rc tags
with open('../../.release') as f:
    version = f.readline().strip().split("=")[1]
    release = version

# -- General configuration ---------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html

# Check if we're running on Read the Docs' servers
read_the_docs_build = os.environ.get('READTHEDOCS', None) == 'True'

breathe_projects = {}
doxygen_xml = ""
source_dir = "../../src"

# relative to conf.py
#if read_the_docs_build:
# build doxygen in docs folder
input_dir = '../src'
output_dir = 'build/doxygen'
configureDoxyfile(input_dir, output_dir)
subprocess.call('mkdir -p ' + output_dir, cwd="..", shell=True)
subprocess.call('doxygen', cwd="..", shell=True)
breathe_projects['SkaPstStat'] = '../' + output_dir + '/xml'
doxygen_xml = '../' + output_dir + '/xml'
#else:
#    doxygen_xml = "...some/path..."
#    breathe_projects['SkaPstStat'] = doxygen_xml


# Add any Sphinx extension module names here, as strings. They can be
# extensions coming with Sphinx (named 'sphinx.ext.*') or your custom
# ones.
extensions = [
    'breathe',
    'exhale',
    'recommonmark',
    'sphinxcontrib.plantuml',
]

source_suffix = [".rst", '.md']

from recommonmark.parser import CommonMarkParser
source_parsers = {".md": CommonMarkParser }

# Automatically generate autodoc_doxygen targets
autodoc_default_flags = ['members']

# Automatically generate stub pages
# autosummary_generate = True

cpp_id_attributes = ["__host__", "__device__", "EIGEN_DEVICE_FUNC"]

# Breathe Config

breathe_default_project = "SkaPstStat"
breathe_default_members = ("members", "undoc-members")
breathe_separate_member_pages = True


breathe_projects_source = {
    "SkaPstStat": (source_dir, [
        "apps",
        "ska"
    ])
}

# breathe_doxygen_config_options = { }

breathe_domain_extension = {
    "h": "cpp",
    "cc": "cpp",
    "cu": "cpp"
}

# Exhale Config

exhale_args = {
    "containmentFolder":     "./api",
    "rootFileName":          "library_root.rst",
    "rootFileTitle":         "Application Programming Interface",
    "afterTitleDescription": textwrap.dedent('''
    The ska-pst-stat library provides an API that is used by the applications. This API is described below.
    '''),
    "doxygenStripFromPath":  "../", #"/home/calgray/Code/icrar/leap-accelerate/src", # use src dir
    # Suggested optional arguments
    "createTreeView":        True,
    # TIP: if using the sphinx-bootstrap-theme, you need
    # "treeViewIsBootstrap": True,
    "exhaleExecutesDoxygen": False,
    #"exhaleDoxygenStdin":    "INPUT = ../../src",
    "lexerMapping": {
        r".*\.h": "cpp",
        r".*\.cc": "cpp",
        r".*\.txt": "cmake"
    },
    #"verboseBuild": True,
    "generateBreatheFileDirectives": False
}

# primary_domain = 'cpp'

# higligh_language = 'cpp'

# Extra Config

# Add any paths that contain templates here, relative to this directory.
templates_path = ['_templates']

# List of patterns, relative to source directory, that match files and
# directories to ignore when looking for source files.
# This pattern also affects html_static_path and html_extra_path.
exclude_patterns = []


# -- Options for HTML output -------------------------------------------------

# The theme to use for HTML and HTML Help pages. See the documentation for
# a list of builtin themes.
#
html_theme = 'sphinx_rtd_theme'

# Theme options are theme-specific and customize the look and feel of a theme
# further.  For a list of options available for each theme, see the
# documentation.
#
# html_theme_options = {}

html_context = {
    'favicon': 'img/favicon.ico',
    'logo': 'img/logo.png',
    'theme_logo_only' : True,
    'display_gitlab': True, # Integrate GitLab
    'github_repo': 'ska-pst-stat', #Repository name
    'gitlab_user': 'ska-telescope', #Repository name
    'gitlab_version': 'main',  #Version
    'conf_py_path': '/docs/src/', # Path in the checkout to the docs root
}

# Add any paths that contain custom static files (such as style sheets) here,
# relative to this directory. They are copied after the builtin static files,
# so a file named "default.css" will overwrite the builtin "default.css".
html_static_path = ['_static']

html_css_files = [
    'css/custom.css',
]

## sphinxcontrib.plantuml
plantuml_syntax_error_image = True

plantuml = '/usr/bin/plantuml -Djava.awt.headless=true '
