// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once
#include <cxxreact/JSBigString.h>
#include <future>
#include <string>

namespace Microsoft::ReactNative {

class LocalBundleReader {
 public:
  static std::future<std::string> LoadBundleAsync(std::string bundlePath);
  static std::string LoadBundle(const std::string &bundlePath);
};

class StorageFileBigString : public facebook::react::JSBigString {
 public:
  StorageFileBigString(const std::string &path);
  bool isAscii() const override;
  const char *c_str() const override;
  size_t size() const override;

  void ensure() const;

 private:
  mutable std::future<std::string> m_futureBuffer;
  mutable std::string m_string;
};

} // namespace Microsoft::ReactNative
