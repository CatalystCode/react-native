/**
 * @format
 */
const fs = require('fs');
const path = require('path');

const rnWin32Path = fs.realpathSync(
  path.dirname(require.resolve('@office-iss/react-native-win32/package.json')),
);
const rnwTesterPath = fs.realpathSync(
  path.dirname(require.resolve('react-native-win32-tester/package.json')),
);

module.exports = {
  // WatchFolders is only needed due to the yarn workspace layout of node_modules, we need to watch the symlinked locations separately
  watchFolders: [
    // Include hoisted modules
    path.resolve(__dirname, '../../node_modules'),
    rnwTesterPath,
    rnWin32Path,
  ],

<<<<<<< HEAD
||||||| 811c767bf
  resolver: {
    resolveRequest: require('./metro-react-native-platform').reactNativePlatformResolver(
      {win32: '@office-iss/react-native-win32'},
    ),
  },
=======
  resolver: {
    extraNodeModules: {
      '@office-iss/react-native-win32': rnWin32Path,
      'react-native-win32-tester': rnwTesterPath,
    },
  },

>>>>>>> 64b0f8706de05473456eae6340a4cbcd938baaaa
  transformer: {
    getTransformOptions: async () => ({
      transform: {
        experimentalImportSupport: false,
        inlineRequires: false,
      },
    }),
  },
};

// Check that we have built our JS files before running the bundler, otherwise we'll get a harder to diagnose "Unable to resolve module" error
if (
  !fs.existsSync(
    path.resolve(__dirname, 'Libraries/Components/View/ViewWin32.js'),
  )
) {
  throw new Error(
    '[31m\nThis package must be built before running the bundler.  Did you mean to run "[39m[33myarn build[39m[31m" first?[39m\n',
  );
}
