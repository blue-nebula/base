# first, let's build the software
# this is going to take the most time
FROM alpine:3.17 AS builder

RUN apk add gcc g++ binutils sdl2-dev zlib-dev perl git wget ca-certificates coreutils \
    sdl2_image-dev sdl2_mixer-dev psmisc cmake ninja

RUN mkdir /blue-nebula
WORKDIR /blue-nebula

COPY src/ /blue-nebula/src
COPY config/ /blue-nebula/config
COPY data/ /blue-nebula/data

COPY CMakeLists.txt /blue-nebula/
COPY README.md /blue-nebula/
COPY docker/serverip.patch /serverip.patch

RUN git apply < /serverip.patch && \
    ls -al && \
    mkdir build && \
    cd build && \
    # TODO: server-only build configuration
    cmake .. -DCMAKE_BUILD_TYPE=Release -G Ninja -DCMAKE_INSTALL_PREFIX=/blue-nebula/install -DBUILD_CLIENT=OFF && \
    ninja -v && \
    ninja install


# next, we convert the example server config to a template
# this is done in no time, so we do not have to benefit from Docker's caching a lot there
FROM python:3.10-alpine AS template

COPY docker/generate-servinit-template.py doc/examples/servinit.cfg /

RUN python3 /generate-servinit-template.py /servinit.cfg > /servinit.tmpl


# finally, let's build the final runtime image (doesn't need all of the build dependencies)
FROM alpine:3.17

LABEL maintainer="https://blue-nebula.org"

RUN apk add tini && \
    apk add --repository=https://dl-cdn.alpinelinux.org/alpine/edge/testing dockerize

RUN adduser -S -D -h /blue-nebula blue-nebula
WORKDIR /blue-nebula

COPY --from=template /servinit.tmpl /
COPY --from=builder /blue-nebula/install /blue-nebula/install

# keep the directory read-only but allow writing to the file
# this is needed since dockerize runs as blue-nebula and needs to modify the file
# we might want to run dockerize separately and then run the process as blue-nebula
RUN mkdir /blue-nebula/.blue-nebula && \
    install -o blue-nebula -m 0644 /dev/null /blue-nebula/.blue-nebula/servinit.cfg

# install runtime dependencies
RUN apk add libstdc++ libgcc

# keep files copied from elsewhere read-only to prevent the process from writing files
USER blue-nebula

EXPOSE 28799/udp 28800/udp 28801/udp 28802/udp

ENTRYPOINT ["/sbin/tini", "--"]

CMD dockerize -template /servinit.tmpl:/blue-nebula/.blue-nebula/servinit.cfg install/bin/blue-nebula_server_linux
