#!/usr/bin/env bash

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd $DIR/..

DOCKER_IMAGE=${DOCKER_IMAGE:-helpforcancer/helpforcancerd-develop}
DOCKER_TAG=${DOCKER_TAG:-latest}

BUILD_DIR=${BUILD_DIR:-.}

rm docker/bin/*
mkdir docker/bin
cp $BUILD_DIR/src/helpforcancerd docker/bin/
cp $BUILD_DIR/src/helpforcancer-cli docker/bin/
cp $BUILD_DIR/src/helpforcancer-tx docker/bin/
strip docker/bin/helpforcancerd
strip docker/bin/helpforcancer-cli
strip docker/bin/helpforcancer-tx

docker build --pull -t $DOCKER_IMAGE:$DOCKER_TAG -f docker/Dockerfile docker
