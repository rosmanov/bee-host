# Copr srpm generator.
#
# This Makefile is invoked as follows:
# make -f <cloned_repodir>/.copr/Makefile srpm outdir="<outdir>" spec="<spec_path>"
#
# See https://docs.pagure.org/copr.copr/user_documentation.html#make-srpm

ARCH := $(shell uname -m)

srpm:
	command -v cmake >/dev/null || dnf install -y cmake
	command -v gcc >/dev/null || dnf install -y gcc
	command -v git >/dev/null || dnf install -y git
	mkdir -p $(outdir)
	CPACK_RPM_PACKAGE_SOURCES=ON CPACK_GENERATOR=RPM ./build-linux-$(ARCH).sh -b Release && cp -a *.src*rpm $(outdir)
