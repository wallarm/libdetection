FROM debian:jessie

RUN apt-get update && \
    apt-get -y install git cmake libcunit1-dev bison re2c && \
    mkdir -p /opt/libwallarmmisc

RUN git clone https://github.com/wallarm/libwallarmmisc.git /opt/libwallarmmisc

RUN cd /opt/libwallarmmisc && \
    ./config -DCMAKE_INSTALL_PREFIX=/usr/local && \
    make -C build && \
    make -C build install

RUN mkdir -p /opt/libdetection
WORKDIR /opt/libdetection
ADD ./ .
RUN ./config && \
    cd build && \
    make && \
    export LD_LIBRARY_PATH=/usr/local/lib && \ 
    echo '123e2union select-id FRoM users--a-' |./perf/libdetection_perf -evvv && \
    echo '123; DROP DATABASE users; ' |./perf/libdetection_perf -evvv
