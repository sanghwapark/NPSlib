FROM centos:centos7

ARG REPO_NAME
ARG APP_VERSION
ARG HCANA_REPO_NAME=hcana
ARG HCANA_VERSION

RUN yum update -q -y

RUN yum -y install epel-release &&\
    yum -y install git && \
    yum -y groupinstall 'Development Tools' && \
    yum -y install gcc-c++ && \
    yum -y install make && \
    yum install -y root && \
    yum install -y which && \
    yum install -y root-montecarlo-eg && \
    yum install -y root-montecarlo-pythia8 && \
    localedef -i en_US -f UTF-8 en_US.UTF-8

ADD https://github.com/Kitware/CMake/releases/download/v3.22.2/cmake-3.22.2-linux-x86_64.tar.gz .
RUN tar -xvf cmake-3.22.2-linux-x86_64.tar.gz && rm cmake-3.22.2-linux-x86_64.tar.gz
RUN mv cmake-3.22.2-linux-x86_64 /usr/local/cmake
ENV PATH="/usr/local/cmake/bin:$PATH"
RUN git clone https://github.com/JeffersonLab/hcana.git --branch ${HCANA_VERSION} /${HCANA_REPO_NAME}-${HCANA_VERSION}
WORKDIR "/${HCANA_REPO_NAME}-${HCANA_VERSION}"
RUN git submodule init && git submodule update
SHELL ["/bin/bash", "-c"]
RUN cmake -DCMAKE_INSTALL_PREFIX=/usr/local/hcana -B build  -S /${HCANA_REPO_NAME}-${HCANA_VERSION}
RUN cmake --build build -j8
RUN cmake --install build
ENV PATH="/usr/local/hcana/bin:/usr/bin/root:$PATH"
ENV LD_LIBRARY_PATH="/usr/local/hcana/lib64:$LD_LIBRARY_PATH"
ENV ANALYZER=/${HCANA_REPO_NAME}-${HCANA_VERSION}/podd
ENV HCANALYZER=/${HCANA_REPO_NAME}-${HCANA_VERSION}
WORKDIR "/${HCANA_REPO_NAME}-${HCANA_VERSION}"
RUN git clone https://github.com/JeffersonLab/NPSlib.git --branch ${APP_VERSION} /NPSlib
WORKDIR "/NPSlib"
RUN cmake -B BUILD -S . -DCMAKE_INSTALL_PREFIX=/usr/local/NPSlib
RUN cmake --build BUILD -j4
RUN cmake --install BUILD
ENV LD_LIBRARY_PATH="/usr/local/NPSlib/lib64:$LD_LIBRARY_PATH"



