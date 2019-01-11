FROM debian:stretch
LABEL maintainer="Helpforcancer Developers <dev@helpforcancer.com>"
LABEL description="Dockerised HelpforcancerCore, built from Travis"

RUN apt-get update && apt-get -y upgrade && apt-get clean && rm -fr /var/cache/apt/*

COPY bin/* /usr/bin/
