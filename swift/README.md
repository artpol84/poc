This is a simple script to build SWIFT MPI application with one command

Example of the output:
```
$ POC_BASE=<path-to-this-poc-repo-clone>
$ module load openmpi
$ $(POC_BASE)/swift/build_swift.sh
Creating 'src' directory ... SUCCESS
Downloading parmetis-4.0.3.tar.gz from http://glaros.dtc.umn.edu/gkhome/fetch/sw/parmetis/parmetis-4.0.3.tar.gz ... SUCCESS
Downloading metis-5.1.0.tar.gz from http://glaros.dtc.umn.edu/gkhome/fetch/sw/metis/metis-5.1.0.tar.gz ... SUCCESS
Downloading fftw-3.3.8.tar.gz from http://www.fftw.org/fftw-3.3.8.tar.gz ... SUCCESS
Downloading gsl-2.5.tar.gz from https://ftp.gnu.org/gnu/gsl/gsl-2.5.tar.gz ... SUCCESS
Downloading hdf5-1.8.20.tar.bz2 from https://support.hdfgroup.org/ftp/HDF5/releases/hdf5-1.8/hdf5-1.8.20/src/hdf5-1.8.20.tar.bz2 ... SUCCESS
Building METIS
    Unpacking metis-5.1.0.tar.gz ... SUCCESS
    METIS required configuration. Configuring ... SUCCESS
    Building METIS ... SUCCESS
    Installing METIS ... SUCCESS
Building ParMETIS
    Unpacking parmetis-4.0.3.tar.gz ... SUCCESS
    ParMETIS required configuration. Configuring ... SUCCESS
    Building ParMETIS ... SUCCESS
    Installing ParMETIS ... SUCCESS
Building HDF5
    Unpacking hdf5-1.8.20.tar.bz2 ... SUCCESS
    No need to run autogen for HDF5. skip
    Configure HDF5 ... SUCCESS
    Building HDF5 ... SUCCESS
    Installing HDF5 ... SUCCESS
Building GSL
    Unpacking gsl-2.5.tar.gz ... SUCCESS
    No need to run autogen for GSL. skip
    Configure GSL ... SUCCESS
    Building GSL ... SUCCESS
    Installing GSL ... SUCCESS
Building FFTW
    Unpacking fftw-3.3.8.tar.gz ... SUCCESS
    No need to run autogen for FFTW. skip
    Configure FFTW ... SUCCESS
    Building FFTW ... SUCCESS
    Installing FFTW ... SUCCESS
Building SWIFT
    Clone SWIFT ... SUCCESS
    Checkout SWIFT/3d44fb65ea39b9f7a2a99525f15c4cd464045c38 ... SUCCESS
    Run 'autogen.sh' for SWIFT ... SUCCESS
    Configure SWIFT ... SUCCESS
    Building SWIFT ... SUCCESS
    Installing SWIFT ... SUCCESS
```