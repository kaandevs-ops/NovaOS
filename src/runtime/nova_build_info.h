#ifndef NOVA_BUILD_INFO_H
#define NOVA_BUILD_INFO_H
#ifndef NOVA_VERSION
#define NOVA_VERSION "0.0.12-dev"
#endif
#ifndef NOVA_BUILD_ID
#define NOVA_BUILD_ID "Unavailable (unversioned build)"
#endif
#if defined(__aarch64__)
#define NOVA_NATIVE_ARCH "ARM64"
#else
#define NOVA_NATIVE_ARCH "x86_64"
#endif
#endif
