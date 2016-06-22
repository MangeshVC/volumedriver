## Why docker?

The Open vStorage Volumedriver depends on a number of prerequisites (tools and libraries) to be installed on your system. Automatically installing these inside a docker container via the included Dockerfile makes it very easy to create a clean, correct and up to date environment for your builds.

As our automated builds are also using the same procedure, this is by far the closest you can get to how we build.

## Prerequisites
- obviously you need to install __docker__. See [Install Docker Engine](https://docs.docker.com/engine/installation/) for instructions.
- __git__ to clone our repo
- we assume you're building on a linux system

## Disk space required
- the docker image is about 1.5GB
- a release build needs about 16GB

## Build time

Building the code and running the included testsuite (1,783 tests) takes around 4 hours on our buildslaves.

## Building

1. fetch the code

    In a directory of choice:

	    git clone https://github.com/openvstorage/volumedriver-buildtools
	    git clone https://github.com/openvstorage/volumedriver

    The _volumedriver-buildtools_ repo contains external libraries and tools that are either newer or not present on a standard installation; the _volumedriver_ repo is the codebase of the volumedriver itself.

    Per default you'll get the _dev_ (developement) branch of the code with the latest, greatest changes that may or may not be in a working state. If you want to build a specific version, you need to switch to that codebase first. We're using the ["gitflow workflow"](https://www.atlassian.com/git/tutorials/comparing-workflows/gitflow-workflow) scheme where development happens on "_dev_" and releases are tagged and merged into "_master_".

    To list the available tags (releases), use the

    		git tag

    command. To switch the codebase to a specific release, for example 5.7.0, do:

    		cd volumedriver
    		git checkout tags/5.7.0
    		cd ..

2. create the build environment image

    - for Ubuntu 14.04 based builds:

			docker build -t bldenv volumedriver-buildtools/docker/ubuntu/

    - for Ubuntu 16.04 based builds:

			docker build -t bldenv volumedriver-buildtools/docker/ubuntu1604/

    - for CentOS 7 based builds:

			docker build -t bldenv volumedriver-buildtools/docker/centos/

    - for Arch Linux based builds (note: unsupported and not packaged):

			docker build -t bldenv volumedriver-buildtools/docker/arch/

3. start a build container

	    docker run -d -v ${PWD}:/home/jenkins/workspace \
	              -e UID=${UID} --privileged --name builder -h builder bldenv

     Notes:
     - privileged is needed to allow mounting inside docker; required if you want to run the included code tests
     - inside the container *supervisor* is used to run extra services (eg. rpcbind) required for the included code tests
     - the current working directory is mapped into the container under */home/jenkins/workspace*; allowing access to the build artifacts from outside the container

4.  get a shell inside the container

		docker exec -u jenkins -i -t builder /bin/bash -l

     Note: the username inside the container is *jenkins* which is mapped onto your uid on your host

5. compiling

    First the _volumedriver-buildtools_ needs to be compiled as the _volumedriver_ code needs the resulting libs and tools. This first step is done via the _build-jenkins.sh_ script. It's then followed by the _jenkins-release-dev.sh_ script to create the volumedriver binaries.

    - standard (release) binaries:

			export WORKSPACE=~/workspace
			export RUN_TESTS=no

			cd ${WORKSPACE}/volumedriver-buildtools/src/release/
			./build-jenkins.sh

			cd ${WORKSPACE}
			./volumedriver/src/buildscripts/jenkins-release-dev.sh ${WORKSPACE}/volumedriver

    - rtchecked (debug) binaries:

			export WORKSPACE=~/workspace
			export RUN_TESTS=no

			cd ${WORKSPACE}/volumedriver-buildtools/src/rtchecked/
			./build-jenkins.sh

			cd ${WORKSPACE}
			./volumedriver/src/buildscripts/jenkins-rtchecked-dev.sh ${WORKSPACE}/volumedriver

    Notes:
    - change "*RUN_TESTS=no*" to "*RUN_TESTS=yes*" to run the included test suite. See [running the testsuite](running_included_testsuite.md) for more info.

6. stop & cleanup docker build container

    Thanks to the mapping of our directory inside the docker container, we can stop and remove the build container while all artifacts will still be available.

		docker stop builder && docker rm builder

    To also remove the docker image itself:

		docker rmi bldenv

Binaries can be found under __volumedriver/build__. Resulting volumedriver packages will be in the __volumedriver/build/debian__ or __volumedriver/build/rpm__ subdir.
