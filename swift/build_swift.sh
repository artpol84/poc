#!/bin/bash

SCRIPT_PATH=$0
SCRIPT_DIR=`dirname $0`
. $SCRIPT_DIR/env.sh

SRCDIR=`pwd`/src
BUILDIR=`pwd`/build
LOGNAME="SWIFT_BUILD_SCRIPT.log"

function get_package()
{
    URL=$1
    FNAME=`basename $URL`
    if [ -f $FNAME ]; then
        echo "No need to download $FNAME"
    else
        echo "Downloading $FNAME from $URL"
        wget $URL
    fi
}

function download()
{
    if [ ! -d ./src ]; then
        echo "Creating 'src' directory"
        mkdir -p src
    else
        echo "Directory 'src' already exists"
    fi
    cd $SRCDIR
    get_package $PARMETIS_URL
    get_package $METIS_URL
    get_package $FFTW_URL
    get_package $GSL_URL
    get_package $HDF5_URL
    cd ../
}

function unpack()
{
    URL=$1
    FNAME=`basename $URL`
    if [ ! -f $FNAME ]; then
        echo "    Need src/$FNAME to proceed: "`pwd`
        exit 1
    fi
    DIRNAME=${FNAME%%.tar.*}
    if [ ! -d "$DIRNAME" ]; then
        echo -n "    Unpacking $FNAME ... "
        tar -xf $FNAME
        echo "SUCCESS"
    fi

    if [ ! -d "$DIRNAME" ]; then
        echo "    FAILED to unpack $FNAME to $DIRNAME"
        exit 1
    fi
}

function check_status()
{
    ret="$1"
    DONT_ERR_EXIT="$2"
    if [ "$ret" = "0" ]; then
        echo "SUCCESS"
    else
        echo "FAILURE"
        echo "Command line: $CMDLINE"
        echo "Output:"
        cat $LOGNAME
        if [ -n "$DONT_ERR_EXIT" ]; then
            # Caller wants to do some cleanup
            return 1
        else
            exit 1
        fi
    fi
    return 0
}

function run_cmd()
{
    DESCR="$1"
    CMDLINE="$2"
    DONT_ERR_EXIT=$3

    echo -n "$DESCR ... "
    $CMDLINE >$LOGNAME 2>&1
    check_status "$?" "$DONT_ERR_EXIT"
    return "$?"
}

function package_prep()
{
    URL="$1"
    PKGNAME="$2"
    PAR_MAKE=""
    if [ "$3" = "parallel_make" ]; then
        PAR_MAKE="-j30"
    fi
    FNAME=`basename $URL`
    DIRNAME=${FNAME%%.tar.*}
    echo "Building $PKGNAME"
    unpack $URL
}

function build_metis_int()
{
    package_prep "$1" "$2"
    cd $DIRNAME
    is_configured=`ls ./build`
    if [ -z "$is_configured" ]; then
        DESCR="    $PKGNAME required configuration. Configuring"
        CMDLINE="make config shared=1 prefix=$BUILDIR/"
        run_cmd "$DESCR" "$CMDLINE" DO_NOT_ERR_EXIT
        if [ "$?" != "0" ]; then
            rm -Rf `pwd`/build/*
            exit 1
        fi
    else
        echo "    $PKGNAME is already configured."
    fi
    run_cmd "    Building $PKGNAME"   "make $PAR_MAKE"
    run_cmd "    Installing $PKGNAME" "make install $PAR_MAKE"
    cd ..
}

function build_metis()
{
    build_metis_int "$METIS_URL" "METIS" parallel_make
}

function build_parmetis()
{
    build_metis_int "$PARMETIS_URL" "ParMETIS" parallel_make
}

function run_autogen()
{
    if [ -f "configure" ]; then
        echo "    No need to run autogen for $PKGNAME. skip"
        return
    fi

    binary=`ls autogen*`
    run_cmd "    Run '$binary' for $PKGNAME" "./$binary"
}


function config_w_automake()
{
    PARAMS="$1"
    if [ -f "config.log" ]; then
        chk=`cat config.log | grep "configure: exit 1"`
        if [ -z "$chk" ]; then
            echo "    $PKGNAME is allready configured. Skip"
            return
        else
            echo "    Previous configuration of $PKGNAME failed. Try again"
            make distclean > /dev/null 2>1
        fi
    fi
        
    run_cmd "    Configure $PKGNAME" \
        "./configure --prefix=$BUILDIR/ $PARAMS" \
        DO_NOT_ERR_EXIT
    if [ "$?" != "0" ]; then
        rm config.log
        exit 1  
    fi
}


function build_w_automake()
{
    cd $DIRNAME
    run_autogen
    config_w_automake "$1"
    run_cmd "    Building $PKGNAME"   "make $PAR_MAKE"
    run_cmd "    Installing $PKGNAME" "make install $PAR_MAKE"
    cd ..    
}

function build_hdf5()
{
    package_prep "$HDF5_URL" "HDF5" parallel_make 
    export LDFLAGS="-lm"
   
    build_w_automake ""
}

function build_gsl()
{
    package_prep "$GSL_URL" "GSL" parallel_make 
    build_w_automake ""
}

function build_fftw()
{
    package_prep "$FFTW_URL" "FFTW" parallel_make
    build_w_automake "--enable-shared=yes"
}

function build_swift()
{
    PKGNAME="SWIFT"
    PAR_MAKE="-j30"
    DIRNAME=swiftsim
    echo "Building $PKGNAME"
    
    if [ -d "$DIRNAME" ]; then
        echo "    $PKGNAME already cloned. skip"
    else
        run_cmd "    Clone $PKGNAME" \
            "git clone $SWIFT_URL"
    fi
    cd $DIRNAME
    run_cmd "    Checkout $PKGNAME/$SWIFT_COMMIT_HASH" \
        "git checkout $SWIFT_COMMIT_HASH"
    cd ../
    build_w_automake "--with-parmetis=$BUILDIR --with-metis=$BUILDIR --with-fftw=$BUILDIR --with-hdf5=$BUILDIR/bin/h5cc --with-gsl=$BUILDIR --with-tbbmalloc"
}

download
cd $SRCDIR
build_metis
build_parmetis
build_hdf5
build_gsl
build_fftw
build_swift
#    unpack $PARMETIS_URL
#    unpack $METIS_URL
#    unpack $FFTW_URL
#   unpack $GSL_URL
#    unpack $HDF5_URL
cd ..
