# Changelog

## [2.3.0](https://github.com/Julusian/node-jpeg-turbo/compare/v2.2.0...v2.3.0) (2025-09-16)


### Features

* update libjpeg-turbo to v3.1.2 ([ff5d104](https://github.com/Julusian/node-jpeg-turbo/commit/ff5d104cb23ff0be58998e302a20367b5dcb351f))


### Bug Fixes

* Use sha256 for the hash verification of the libjpeg-turbo tarball ([#30](https://github.com/Julusian/node-jpeg-turbo/issues/30)) ([d84af01](https://github.com/Julusian/node-jpeg-turbo/commit/d84af01684bc28dc7f3fe47459cd88e29d3bad51))

## [2.2.0](https://github.com/Julusian/node-jpeg-turbo/compare/v2.1.0...v2.2.0) (2024-09-06)


### Features

* add windows-arm build ([f323c89](https://github.com/Julusian/node-jpeg-turbo/commit/f323c89d361df81a56cdf8036cd3f51e3ab624be))
* update libjpeg-turbo to v3.0.1 ([2430ae0](https://github.com/Julusian/node-jpeg-turbo/commit/2430ae07ff645a65bc907389159b6e7923f839ef))


### Bug Fixes

* build jpegturbo as shared lib on linux ([796f4c0](https://github.com/Julusian/node-jpeg-turbo/commit/796f4c0d09c98ab544df1ed9a0d2af7d2c227334))
* pass CMAKE_OSX_ARCHITECTURES to libjpeg-turbo ([e42c558](https://github.com/Julusian/node-jpeg-turbo/commit/e42c558d219cf22a5812cc9749dc47e5f33ba166))
* remove binding.gyp, to stop electron-builder rebuilding the library ([eb1ce2d](https://github.com/Julusian/node-jpeg-turbo/commit/eb1ce2d66896957e6b6acd492b0173e280cf94e6))
