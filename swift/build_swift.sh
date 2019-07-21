#!/bin/bash
#
# Copyright (C) 2019      Mellanox Technologies, Inc.
#                         All rights reserved.
#                         Written by Artem Y. Polyakov
# $COPYRIGHT$
#
# Additional copyrights may follow
#
# $HEADER$
#

SCRIPT_PATH=$0
SCRIPT_DIR=`dirname $0`

if [ -f "swift_env.sh" ]; then
    # If user defined a local env - use it
    . swift_env.sh
else
    # Use the default env file
    . $SCRIPT_DIR/swift_env.sh
fi

SRCDIR=`pwd`/src
BUILDIR=`pwd`/build
LOGNAME="/tmp/SWIFT_BUILD_SCRIPT-$$.log"


export CFLAGS="-g -O3"

function check_status()
{
    silent="$1"
    ret="$2"
    DONT_ERR_EXIT="$3"
    if [ "$ret" = "0" ]; then
        if [ "$silent" = "0" ] ;then
            echo "SUCCESS"
        fi
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
    shift
    if [ "$1" = "DO_NOT_ERR_EXIT" ]; then
        DONT_ERR_EXIT=$1
        shift
    fi
    CMDLINE="$@"

    silent=0
    if [ "$DESCR" != "-" ]; then
        echo -n "$DESCR ... "
    else
        silent=1
    fi

    # Run the command
    $CMDLINE >$LOGNAME 2>&1
    check_status $silent "$?" "$DONT_ERR_EXIT"
    return "$?"
}


function get_package()
{
    URL=$1
    FNAME=`basename $URL`
    if [ -f $FNAME ]; then
        echo "No need to download $FNAME"
    else
        run_cmd "Downloading $FNAME from $URL" \
            wget $URL
    fi
}

function download()
{
    if [ ! -d ./src ]; then
        run_cmd "Creating 'src' directory" \
            mkdir -p src
    else
        echo "Directory 'src' already exists"
    fi
    run_cmd "-" cd $SRCDIR
    get_package $PARMETIS_URL
    get_package $METIS_URL
    get_package $FFTW_URL
    get_package $GSL_URL
    get_package $HDF5_URL
    run_cmd "-" cd ../
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


function unpack()
{
    if [ ! -f $FNAME ]; then
        echo "    Need src/$FNAME to proceed: "`pwd`
        exit 1
    fi
    DIRNAME=${FNAME%%.tar.*}

    if [ ! -d "$DIRNAME" ]; then
        run_cmd "    Unpacking $FNAME" tar -xf $FNAME
    fi

    if [ ! -d "$DIRNAME" ]; then
        echo "    FAILED to unpack $FNAME to $DIRNAME"
        exit 1
    fi
}

function build_metis_int()
{
    package_prep "$1" "$2"
    run_cmd "-" cd $DIRNAME
    is_configured=`ls ./build`
    if [ -z "$is_configured" ]; then
        DESCR="    $PKGNAME required configuration. Configuring"
        local conf_append=""
#        if [ "$CC" = "icc" ]; then
#    	    conf_append="CFLAGS=\"-fPIC\""
#    	fi
        run_cmd "$DESCR" DO_NOT_ERR_EXIT \
            make config shared=1 prefix=$BUILDIR/ LDFLAGS="-lm" $conf_append
        if [ "$?" != "0" ]; then
            run_cmd "-" rm -Rf `pwd`/build/*
            exit 1
        fi
    else
        echo "    $PKGNAME is already configured."
    fi
    run_cmd "    Building $PKGNAME"   "make $PAR_MAKE"
    run_cmd "    Installing $PKGNAME" "make install $PAR_MAKE"
    run_cmd "-" cd ..
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
        
    run_cmd "    Configure $PKGNAME" DO_NOT_ERR_EXIT \
        ./configure --prefix=$BUILDIR/ $PARAMS
        
    if [ "$?" != "0" ]; then
        run_cmd "-" rm config.log
        exit 1  
    fi
}


function build_w_automake()
{
    run_cmd "-" cd $DIRNAME
    run_autogen
    config_w_automake "$1"
    run_cmd "    Building $PKGNAME"   "make $PAR_MAKE"
    run_cmd "    Installing $PKGNAME" "make install $PAR_MAKE"
    run_cmd "-" cd ..    
}

function build_hdf5()
{
    package_prep "$HDF5_URL" "HDF5" parallel_make 
    export LDFLAGS="-lm"
   
    build_w_automake "--enable-parallel CC=$MPICC"
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
            git clone $SWIFT_URL
    fi
    run_cmd "-" cd $DIRNAME
    run_cmd "    Checkout $PKGNAME/$SWIFT_COMMIT_HASH" \
        git checkout $SWIFT_COMMIT_HASH

    # Few minor changes
    cat src/engine.c | sed 's/engine_dump_snapshot(.*);//g' > src/engine.c.new
    run_cmd "Patch engine.c" mv src/engine.c.new src/engine.c
    cat examples/main.c | sed 's/engine_dump_snapshot(.*);//g' > examples/main.c.new
    run_cmd "Patch main.c" mv examples/main.c.new examples/main.c

    # Guild using existing infra
    run_cmd "-" cd ../
    build_w_automake "--with-parmetis=$BUILDIR --with-metis=$BUILDIR --with-fftw=$BUILDIR --with-hdf5=$BUILDIR/bin/h5pcc --with-gsl=$BUILDIR --with-tbbmalloc \
                --with-hydro=anarchy-du --with-kernel=quintic-spline --with-subgrid=EAGLE --disable-hand-vec CC=$CC MPICC=$MPICC"
# CFLAGS=-qopt-zmm-usage=high"
  
}

download

run_cmd "-" cd $SRCDIR

build_metis
build_parmetis
build_hdf5
build_gsl
build_fftw
build_swift

run_cmd "-" cd ..
run_cmd "-" rm -f $LOGNAME
