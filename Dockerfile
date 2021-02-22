FROM ubuntu:16.04
MAINTAINER Bing Liu liub@mail.bnu.edu.cn

RUN apt-get update
RUN apt-get -y install git wget gcc make automake autoconf cmake bzip2 g++

RUN git clone https://github.com/cosmo-team/cosmo/
RUN cd cosmo/ && git checkout VARI && mkdir 3rd_party_src && mkdir -p 3rd_party_inst/boost && cd 3rd_party_src
RUN cd /cosmo/3rd_party_src && git clone https://github.com/refresh-bio/KMC \
	&& git clone https://github.com/cosmo-team/sdsl-lite.git \
	&& git clone https://github.com/stxxl/stxxl \
	&& git clone https://github.com/eile/tclap

RUN cd /cosmo/3rd_party_src && wget http://sourceforge.net/projects/boost/files/boost/1.54.0/boost_1_54_0.tar.bz2 > /dev/null 2>&1 && \
	tar -xjf boost_1_54_0.tar.bz2

RUN cd /cosmo/3rd_party_src/boost_1_54_0 &&  ./bootstrap.sh --prefix=../../3rd_party_inst/boost
RUN cd /cosmo/3rd_party_src/boost_1_54_0 && ./b2 install; exit 0
RUN cd /cosmo/3rd_party_src/sdsl-lite/ && \
	git rm -rf external/googletest && git rm -rf external/libdivsufsort && \
	git submodule add -f https://github.com/google/googletest.git external/googletest && \
	git submodule add -f https://github.com/simongog/libdivsufsort.git external/libdivsufsort && \
	sed '26c add_subdirectory(googletest/googletest)' external/CMakeLists.txt >tmp && mv tmp ./external/CMakeLists.txt && \
	sh install.sh /cosmo/3rd_party_inst/

RUN cd /cosmo/3rd_party_src/stxxl && mkdir build && cd build && \
	cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/cosmo/3rd_party_inst/ -DBUILD_STATIC_LIBS=ON && \
	make && make install

RUN cd /cosmo/3rd_party_src/KMC && make && cd ..

RUN cd /cosmo/3rd_party_src/tclap/ && autoreconf -fvi && ./configure --prefix=/cosmo/3rd_party_inst/ && make && make install; exit 0
RUN cd /cosmo/ && make

ENV LD_LIBRARY_PATH /cosmo/3rd_party_inst/boost/lib:$LD_LIBRARY_PATH
ENV PATH /cosmo:$PATH


RUN cd ~ && mkdir data

VOLUME data