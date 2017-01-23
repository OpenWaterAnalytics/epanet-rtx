# dockerfile for epanet-rtx library and associated tools.
# this file is used for testing and pre-flight.

FROM ubuntu:16.04
MAINTAINER Sam Hatchett <sam@citilogics.com>

RUN apt-get update && \
	apt-get -y install \
	libsqlite3-dev \
	cmake \
	libiodbc2-dev \
	tdsodbc \
	libboost-all-dev \
	libcurl4-openssl-dev \
	g++ \
	openssl \
	libssl-dev \
	git \
	curl

WORKDIR /opt/epanet-rtx

# build microsoft's cpprestsdk - library for rest i/o
RUN git clone https://github.com/Microsoft/cpprestsdk.git casablanca && \
	cd casablanca/Release && \
	sed -e 's/ -Wcast-align//g' -i CMakeLists.txt  && \
	mkdir build.release && \
	cd build.release && \
	cmake .. -DCMAKE_BUILD_TYPE=Release -DBUILD_SAMPLES=0 -DBUILD_TESTS=0 && \
	make && make install

# copy build resources
COPY src src
COPY build build
COPY examples examples


## build LINK service (rtx-based tier-2 data synchronization service)
RUN cd examples/LINK-service/cmake && \
    mkdir build && \
    cd build && \
    cmake .. && \
    make



