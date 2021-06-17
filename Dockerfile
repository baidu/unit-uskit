FROM ubuntu:18.04

WORKDIR /unit-uskit

COPY . /unit-uskit

RUN sed -i s@/archive.ubuntu.com/@/mirrors.aliyun.com/@g /etc/apt/sources.list

RUN sh deps.sh ubuntu

RUN rm -rf _build && mkdir _build && cd _build && cmake .. && make -j8

EXPOSE 8888