module.exports = {
  extends: ['@rnw-scripts'],
  parserOptions: { tsconfigRootDir: __dirname },
  globals: {
    $: 'readonly',
    browser: 'readonly',
    expect: 'readonly',
  },
};
