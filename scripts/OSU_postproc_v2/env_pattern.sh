#!/bin/bash

OMPI_BASE=<ompi-install-path>
UCX_BASE=<ucx-install-path>
UCX_LIB_BASE=${UCX_BASE}/lib

export PATH="$OMPI_BASE/bin/:$PATH"
export LD_LIBRARY_PATH="$OMPI_BASE/bin/:$LD_LIBRARY_PATH"

export UCX_PROFILE_MODE="accum"
export UCX_PROFILE_FILE="<filename>/%e_%h.log"