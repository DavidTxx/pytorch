#include <c10d/GlooDeviceFactory.hpp>

#include <stdlib.h>

#include <c10/util/Exception.h>

#if GLOO_HAVE_TRANSPORT_TCP
#include <gloo/transport/tcp/device.h>
#endif

#if GLOO_HAVE_TRANSPORT_TCP_TLS
#include <gloo/transport/tcp/tls/device.h>
#endif

#if GLOO_HAVE_TRANSPORT_UV
#include <gloo/transport/uv/device.h>
#endif

// On Linux, check that the tcp transport is available.
#ifdef __linux__
#if !GLOO_HAVE_TRANSPORT_TCP
#error "Expected the tcp transport to be available on Linux."
#endif
#endif

// On macOS, check that the uv transport is available.
#ifdef __APPLE__
#if !GLOO_HAVE_TRANSPORT_UV
#error "Expected the uv transport to be available on macOS."
#endif
#endif

namespace c10d {

C10_DEFINE_SHARED_REGISTRY_WITHOUT_WARNING(
    GlooDeviceRegistry,
    ::gloo::transport::Device,
    const std::string& /* interface */,
    const std::string& /* hostname */);

#if GLOO_HAVE_TRANSPORT_TCP
static std::shared_ptr<::gloo::transport::Device> makeTCPDevice(
    const std::string& interface,
    const std::string& hostname) {
  TORCH_CHECK(
      !interface.empty() || !hostname.empty(),
      "GlooDeviceFactory::makeTCPDevice(): interface or hostname "
      "can't be empty");

  ::gloo::transport::tcp::attr attr;
  if (!interface.empty()) {
    attr.iface = interface;
  } else {
    attr.hostname = hostname;
  }
  return ::gloo::transport::tcp::CreateDevice(attr);
}

// Registry priority is per key identifier. We register TCP to `LINUX` for
// the flexibility of other application to override by priority. Register
// TCP to `TCP` for env "GLOO_DEVICE_TRANSPORT" override.
C10_REGISTER_CREATOR(GlooDeviceRegistry, LINUX, makeTCPDevice);
C10_REGISTER_CREATOR(GlooDeviceRegistry, TCP, makeTCPDevice);
#endif

#if GLOO_HAVE_TRANSPORT_TCP_TLS

static std::string env_to_string(const char* chars) {
  return chars == nullptr ? std::string("") : std::string(chars);
}

static std::shared_ptr<::gloo::transport::Device> makeTCPTLSDevice(
    const std::string& interface,
    const std::string& hostname) {
  TORCH_CHECK(
      !interface.empty() || !hostname.empty(),
      "GlooDeviceFactory::makeTCPTLSDevice(): interface or hostname "
      "can't be empty");

  ::gloo::transport::tcp::attr attr;
  if (!interface.empty()) {
    attr.iface = interface;
  } else {
    attr.hostname = hostname;
  }
  const auto pkey = env_to_string(std::getenv("GLOO_DEVICE_TRANSPORT_TCP_TLS_PKEY"));
  const auto cert = env_to_string(std::getenv("GLOO_DEVICE_TRANSPORT_TCP_TLS_CERT"));
  const auto caFile = env_to_string(std::getenv("GLOO_DEVICE_TRANSPORT_TCP_TLS_CA_FILE"));
  const auto caPath = env_to_string(std::getenv("GLOO_DEVICE_TRANSPORT_TCP_TLS_CA_PATH"));
  return ::gloo::transport::tcp::tls::CreateDevice(attr, pkey, cert, caFile, caPath);
}

C10_REGISTER_CREATOR(GlooDeviceRegistry, TCP_TLS, makeTCPTLSDevice);
#endif

#if GLOO_HAVE_TRANSPORT_UV
static std::shared_ptr<::gloo::transport::Device> makeUVDevice(
    const std::string& interface,
    const std::string& hostname) {
  TORCH_CHECK(
      !interface.empty() || !hostname.empty(),
      "GlooDeviceFactory::makeUVDevice(): interface or hostname "
      "can't be empty");

  ::gloo::transport::uv::attr attr;
  if (!interface.empty()) {
    attr.iface = interface;
  } else {
    attr.hostname = hostname;
  }
  return ::gloo::transport::uv::CreateDevice(attr);
}

// Registry priority is per key identifier. We register UV to `APPLE` for
// the flexibility of other application to override by priority. Register
// UV to `UV` for env "GLOO_DEVICE_TRANSPORT" override.
C10_REGISTER_CREATOR(GlooDeviceRegistry, APPLE, makeUVDevice);
C10_REGISTER_CREATOR(GlooDeviceRegistry, UV, makeUVDevice);
#endif

static const char* glooDeviceTransport = getenv("GLOO_DEVICE_TRANSPORT");

std::shared_ptr<::gloo::transport::Device> GlooDeviceFactory::
    makeDeviceForInterface(const std::string& interface) {
  if (glooDeviceTransport) {
    return GlooDeviceRegistry()->Create(glooDeviceTransport, interface, "");
  }

#ifdef __linux__
  return GlooDeviceRegistry()->Create("LINUX", interface, "");
#endif

#ifdef __APPLE__
  return GlooDeviceRegistry()->Create("APPLE", interface, "");
#endif

  throw std::runtime_error("makeDeviceForInterface(): unsupported gloo device");
}

std::shared_ptr<::gloo::transport::Device> GlooDeviceFactory::
    makeDeviceForHostname(const std::string& hostname) {
  if (glooDeviceTransport) {
    return GlooDeviceRegistry()->Create(glooDeviceTransport, "", hostname);
  }

#ifdef __linux__
  return GlooDeviceRegistry()->Create("LINUX", "", hostname);
#endif

#ifdef __APPLE__
  return GlooDeviceRegistry()->Create("APPLE", "", hostname);
#endif

  throw std::runtime_error("makeDeviceForHostname(): unsupported gloo device");
}

} // namespace c10d
