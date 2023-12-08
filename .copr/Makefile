# Copr srpm generator.
#
# This Makefile is invoked as follows:
# make -f <cloned_repodir>/.copr/Makefile srpm outdir="<outdir>" spec="<spec_path>"
#
# See https://docs.pagure.org/copr.copr/user_documentation.html#make-srpm

ARCH := $(shell uname -m)

srpm:
	dnf install -y cmake gcc git
	mkdir -p "$(outdir)"
	CPACK_INSTALLED_DIRECTORIES="$(shell pwd);./" \
		CPACK_GENERATOR=RPM \
		CPACK_RPM_PACKAGE_SOURCES=ON \
		sh ./build-linux-$(ARCH).sh -b Release && cp -a beectl*.src.rpm "$(outdir)"
