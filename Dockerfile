FROM debian:jessie

RUN apt-get update && \
    apt-get -y install autotools-dev automake libtool autoconf git cmake libcunit1-dev bison re2c && \
    mkdir -p /opt/avl

RUN git clone git://git.fruit.je/avl /opt/avl

RUN cd /opt/avl && \
    autoreconf -i && \
    ./configure --prefix=/usr/local && \
    make && \
    make install 

RUN mkdir -p /opt/libdetection
ADD ./ /opt/libdetection/
RUN cd /opt/libdetection && \
    ./configure && \
    cd build && \
    make && \
    export LD_LIBRARY_PATH=/usr/local/lib && \ 
    echo '123e2union select-id FRoM users--a-' |./perf/perf -evvv && \ 
    echo '123; DROP DATABASE users; ' |./perf/perf -evvv 
