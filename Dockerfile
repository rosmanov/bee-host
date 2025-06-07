FROM --platform=linux/amd64 debian:bullseye AS base
ARG BUILD_TYPE=Release
ENV BUILD_TYPE=${BUILD_TYPE}
ARG BEECTL_BUILD_DIR=/src/build
ENV BEECTL_BUILD_DIR=${BEECTL_BUILD_DIR}
ENV DEBIAN_FRONTEND=noninteractive
WORKDIR /src
RUN apt-get update && \
    apt-get install -y --no-install-recommends \
    ca-certificates && \
    rm -rf /var/lib/apt/lists/*

FROM base AS x86-multilib
RUN dpkg --add-architecture i386 && \
    apt-get update && \
    apt-get install -y --no-install-recommends \
    build-essential \
    gcc-multilib \
    g++-multilib \
    libc6-dev:i386 \
    mingw-w64 \
    cmake make ninja-build \
    git curl nsis rpm && \
    rm -rf /var/lib/apt/lists/*
COPY . .

FROM x86-multilib AS build-linux-amd64
RUN bash build.sh CMake/Toolchain-Linux-amd64.cmake -d "${BEECTL_BUILD_DIR}" -b "${BUILD_TYPE}"

FROM x86-multilib AS build-linux-i386
RUN bash build.sh CMake/Toolchain-Linux-i386.cmake -d "${BEECTL_BUILD_DIR}" -b "${BUILD_TYPE}"

FROM x86-multilib AS build-windows-i686
RUN bash build.sh CMake/Toolchain-Windows-i686.cmake -d "${BEECTL_BUILD_DIR}" -b "${BUILD_TYPE}"

FROM x86-multilib AS build-windows-amd64
RUN bash build.sh CMake/Toolchain-Windows-amd64.cmake -d "${BEECTL_BUILD_DIR}" -b "${BUILD_TYPE}"

FROM base AS build-ppc64le-cross
RUN apt-get update && \
    apt-get install -y --no-install-recommends \
    build-essential cmake make ninja-build git curl rpm \
    crossbuild-essential-ppc64el && \
    rm -rf /var/lib/apt/lists/*
COPY . .
RUN bash build.sh CMake/Toolchain-Linux-ppc64le.cmake -d "${BEECTL_BUILD_DIR}" -b "${BUILD_TYPE}"

FROM base AS build-arm-cross
RUN apt-get update && \
    apt-get install -y --no-install-recommends \
    crossbuild-essential-armhf \
    cmake make ninja-build git curl rpm && \
    rm -rf /var/lib/apt/lists/*
COPY . .
RUN bash build.sh CMake/Toolchain-Linux-arm.cmake -d "${BEECTL_BUILD_DIR}" -b "${BUILD_TYPE}"

FROM base AS build-aarch64-cross
RUN apt-get update && \
    apt-get install -y --no-install-recommends \
    gcc-aarch64-linux-gnu g++-aarch64-linux-gnu \
    cmake make ninja-build git curl rpm && \
    apt-get clean && rm -rf /var/lib/apt/lists/*
COPY . .
RUN bash build.sh CMake/Toolchain-Linux-aarch64.cmake -d "${BEECTL_BUILD_DIR}" -b "${BUILD_TYPE}"

FROM base AS final
WORKDIR /artifacts
COPY --from=build-linux-amd64     ${BEECTL_BUILD_DIR}/*.rpm ./
COPY --from=build-linux-amd64     ${BEECTL_BUILD_DIR}/*.deb ./
COPY --from=build-linux-amd64     ${BEECTL_BUILD_DIR}/*.zip ./
COPY --from=build-linux-amd64     ${BEECTL_BUILD_DIR}/*.tgz ./
COPY --from=build-linux-amd64     ${BEECTL_BUILD_DIR}/*.tar.gz ./

COPY --from=build-aarch64-cross "${BEECTL_BUILD_DIR}"/*.rpm ./
COPY --from=build-aarch64-cross "${BEECTL_BUILD_DIR}"/*.deb ./
COPY --from=build-aarch64-cross "${BEECTL_BUILD_DIR}"/*.zip ./
COPY --from=build-aarch64-cross "${BEECTL_BUILD_DIR}"/*.tgz ./
COPY --from=build-aarch64-cross "${BEECTL_BUILD_DIR}"/*.tar.gz ./

COPY --from=build-linux-i386      ${BEECTL_BUILD_DIR}/*.rpm ./
COPY --from=build-linux-i386      ${BEECTL_BUILD_DIR}/*.deb ./
COPY --from=build-linux-i386      ${BEECTL_BUILD_DIR}/*.zip ./
COPY --from=build-linux-i386      ${BEECTL_BUILD_DIR}/*.tgz ./
COPY --from=build-linux-i386      ${BEECTL_BUILD_DIR}/*.tar.gz ./

COPY --from=build-ppc64le-cross   ${BEECTL_BUILD_DIR}/*.rpm ./
COPY --from=build-ppc64le-cross   ${BEECTL_BUILD_DIR}/*.deb ./
COPY --from=build-ppc64le-cross   ${BEECTL_BUILD_DIR}/*.zip ./
COPY --from=build-ppc64le-cross   ${BEECTL_BUILD_DIR}/*.tgz ./
COPY --from=build-ppc64le-cross   ${BEECTL_BUILD_DIR}/*.tar.gz ./
COPY --from=build-arm-cross       ${BEECTL_BUILD_DIR}/*.rpm ./
COPY --from=build-arm-cross       ${BEECTL_BUILD_DIR}/*.deb ./
COPY --from=build-arm-cross       ${BEECTL_BUILD_DIR}/*.zip ./
COPY --from=build-arm-cross       ${BEECTL_BUILD_DIR}/*.tgz ./
COPY --from=build-arm-cross       ${BEECTL_BUILD_DIR}/*.tar.gz ./

COPY --from=build-windows-i686    ${BEECTL_BUILD_DIR}/*.zip ./
COPY --from=build-windows-i686    ${BEECTL_BUILD_DIR}/*.exe ./
COPY --from=build-windows-amd64   ${BEECTL_BUILD_DIR}/*.zip ./
COPY --from=build-windows-amd64   ${BEECTL_BUILD_DIR}/*.exe ./
