#!/bin/bash

#####################################################################################################
# General parameters
#####################################################################################################
SWIFT_INST_DIR=""
SWIFT_HOSTS=""

#####################################################################################################
# SWIFT software stack info
#####################################################################################################


PARMETIS_URL=http://glaros.dtc.umn.edu/gkhome/fetch/sw/parmetis/parmetis-4.0.3.tar.gz
METIS_URL=http://glaros.dtc.umn.edu/gkhome/fetch/sw/metis/metis-5.1.0.tar.gz
FFTW_URL=http://www.fftw.org/fftw-3.3.8.tar.gz
GSL_URL=https://ftp.gnu.org/gnu/gsl/gsl-2.5.tar.gz
HDF5_URL=https://support.hdfgroup.org/ftp/HDF5/releases/hdf5-1.8/hdf5-1.8.20/src/hdf5-1.8.20.tar.bz2
SWIFT_URL=https://gitlab.cosma.dur.ac.uk/swift/swiftsim.git
#SWIFT_COMMIT_HASH=3d44fb65ea39b9f7a2a99525f15c4cd464045c38
SWIFT_COMMIT_HASH=

#####################################################################################################
# Building info
#####################################################################################################

COMPILER=gnu		# gcc or intel
MPI=hpcx		# hpcx, impi or custom
MPI_APPEND=""


if [ "$COMPILER" = "intel" ]; then
    echo "Using Intel Compiler"    
    module load intel/ics-18.0.4
    set -x
    export CC=icc
    set +x
else
    echo "Using GNU compiler"
    set -x
    export CC=gcc
    set +x
fi

if [ "$MPI" = impi ]; then
    echo "Using Intel MPI"    
    module load intel/ics-18.0.4
    set -x
    export MPICC=mpiicc
    set +x
elif [ "$MPI" = "hpcx" ]; then
    if [ "$COMPILER" = intel ]; then
        echo "Using HPCX/icc"
        module load hpcx-icc-mt
    else
        echo "Using HPCX/gcc"    
        module load hpcx-gcc-mt
    fi
    set -x
    export MPICC=mpicc
    set +x
else
    echo "Using custom MPI"
    MPI_BASE=<"<path-to-MPI-installation>"
    export LD_LIBRARY_PATH="$MPI_BASE/lib:$LD_LIBRARY_PATH"
    export PATH="$MPI_BASE/bin:$PATH"
    which mpicc
    set -x
    export MPICC=mpicc
    set +x
fi
