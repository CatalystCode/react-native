/**
 * Copyright (c) Facebook, Inc. and its affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 *
 * @flow strict-local
 * @format
 */

'use strict';

import * as React from 'react';
import StyleSheet from '../../StyleSheet/StyleSheet';
import Text from '../../Text/Text';
import View from '../../Components/View/View';
import Platform from '../../Utilities/Platform';
import LogBoxButton from './LogBoxButton';
import * as LogBoxStyle from './LogBoxStyle';

import type {PressEvent} from '../../Types/CoreEventTypes';
import type {StackFrame} from '../../Core/NativeExceptionsManager';

type Props = $ReadOnly<{|
  frame: StackFrame,
  onPress?: ?(event: PressEvent) => void,
|}>;

function LogBoxInspectorStackFrame(props: Props): React.Node {
  const {frame, onPress} = props;
  const column = frame.column != null && parseInt(frame.column, 10);
  const location =
    getFileName(frame.file) +
    (frame.lineNumber != null
      ? ':' +
        frame.lineNumber +
        (column && !isNaN(column) ? ':' + (column + 1) : '')
      : '');
  return (
    <View style={styles.frameContainer}>
      <LogBoxButton
        backgroundColor={{
          default: 'transparent',
          pressed: onPress ? LogBoxStyle.getBackgroundColor(1) : 'transparent',
        }}
        onPress={onPress}
        style={styles.frame}>
        <Text style={[styles.name, frame.collapse === true && styles.dim]}>
          {frame.methodName}
        </Text>
        <Text
          ellipsizeMode="tail"  // Win32 doesn't support middle
          numberOfLines={2} // Win32 extend to two lines to help mitigate lack of middle ellipsize.
          style={[styles.location, frame.collapse === true && styles.dim]}>
          {location}
        </Text>
      </LogBoxButton>
    </View>
  );
}

function getFileName(file) {
  if (file == null) {
    return '<unknown>';
  }
  const queryIndex = file.indexOf('?');
  return file.substring(
    file.lastIndexOf('/') + 1,
    queryIndex === -1 ? file.length : queryIndex,
  );
}

const styles = StyleSheet.create({
  frameContainer: {
    flexDirection: 'row',
    paddingHorizontal: 15,
  },
  frame: {
    flex: 1,
    paddingVertical: 4,
    paddingHorizontal: 10,
    borderRadius: 5,
  },
  lineLocation: {
    flexDirection: 'row',
  },
  name: {
    color: LogBoxStyle.getTextColor(1),
    fontSize: 14,
    includeFontPadding: false,
    lineHeight: 18,
    fontWeight: '400',
    fontFamily: Platform.select({android: 'monospace', ios: 'Menlo'}),
  },
  location: {
    color: LogBoxStyle.getTextColor(0.8),
    fontSize: 12,
    fontWeight: '300',
    includeFontPadding: false,
    lineHeight: 16,
    paddingLeft: 10,
  },
  dim: {
    color: LogBoxStyle.getTextColor(0.4),
    fontWeight: '300',
  },
  line: {
    color: LogBoxStyle.getTextColor(0.8),
    fontSize: 12,
    fontWeight: '300',
    includeFontPadding: false,
    lineHeight: 16,
  },
});

export default LogBoxInspectorStackFrame;
