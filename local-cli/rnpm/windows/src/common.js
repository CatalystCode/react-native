/**
 * Copyright (c) 2015-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

'use strict';

const fs = require('fs');
const path = require('path');
const semver = require('semver');
const fetch = require('node-fetch');
const HttpsProxyAgent = require('https-proxy-agent');

const proxyUrl = process.env.https_proxy || process.env.HTTPS_PROXY;
const options  = {};
if(proxyUrl) {
  options.agent = new HttpsProxyAgent(proxyUrl);
}

const REACT_NATIVE_PACKAGE_JSON_PATH = function() {
  return path.resolve(
    process.cwd(),
    'node_modules',
    'react-native',
    'package.json'
  );
};

function getLatestVersion() {
  return fetch('https://registry.npmjs.org/react-native-windows?version=latest', options)
    .then(result => result && result.ok && result.json())
    .then(result => result.version)
}

function getMatchingVersion(version) {
  console.log(`Checking for react-native-windows version matching ${version}...`)
  return fetch(`https://registry.npmjs.org/react-native-windows?version=${version}`, options)
    .then(result => {
      if (result && result.ok) {
        return result.json().then(pkg => pkg.version);
      } else {
        return getLatestVersion().then(latestVersion => {
          throw new Error(`Could not find react-native-windows@${version}. ` +
            `Latest version of react-native-windows is ${latestVersion}, try switching to ` +
            `react-native@${semver.major(latestVersion)}.${semver.minor(latestVersion)}.*.`);
        });
      }
    });
}

const getInstallPackage = function (version) {
  let packageToInstall = 'react-native-windows';

  const validVersion = semver.valid(version);
  const validRange = semver.validRange(version);
  if ((validVersion && !semver.gtr(validVersion, '0.26.*')) ||
      (!validVersion && validRange && semver.gtr('0.27.0', validRange))) {
    console.error(
      'Please upgrade react-native to ^0.27 or specify a --windowsVersion that is >=0.27.0'
    );
    process.exit(1);
  }

  if (validVersion || validRange) {
    return getMatchingVersion(version)
      .then(resultVersion => packageToInstall + '@' + resultVersion);
  } else {
    return Promise.resolve(version);
  }
}

const getReactNativeVersion = function () {
  console.log('Reading react-native version from node_modules...');
  if (fs.existsSync(REACT_NATIVE_PACKAGE_JSON_PATH())) {
    const version = JSON.parse(fs.readFileSync(REACT_NATIVE_PACKAGE_JSON_PATH(), 'utf-8')).version;
    return `${semver.major(version)}.${semver.minor(version)}.*`;
  }
}

const getReactNativeAppName = function () {
  console.log('Reading application name from package.json...');
  return JSON.parse(fs.readFileSync('package.json', 'utf8')).name;
}

module.exports = {
  getInstallPackage,
  getReactNativeVersion,
  getReactNativeAppName
}
