/**
 * Copyright (c) 2015-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 *
 * @providesModule TouchableButtonFeedback
 * @flow
 */
'use strict';

const ColorPropType = require('ColorPropType');
const NativeMethodsMixin = require('NativeMethodsMixin');
const PropTypes = require('prop-types');
const React = require('React');
const ReactNativeViewAttributes = require('ReactNativeViewAttributes');
const StyleSheet = require('StyleSheet');
const TimerMixin = require('react-timer-mixin');
const Touchable = require('Touchable');
const TouchableWithoutFeedback = require('TouchableWithoutFeedback');
const UIManager = require('UIManager');
const View = require('View');
const ViewPropTypes = require('ViewPropTypes');

const createReactClass = require('create-react-class');
const ensureComponentIsNative = require('ensureComponentIsNative');
const ensurePositiveDelayProps = require('ensurePositiveDelayProps');
const keyOf = require('fbjs/lib/keyOf');
const merge = require('merge');

import type {Event} from 'TouchableWithoutFeedback';

const DEFAULT_PROPS = {
  activeOpacity: 0.85, // TODO: check this
  activePadding: 2,
  underlayColor: 'black',
};

const PRESS_RETENTION_OFFSET = {top: 20, left: 20, right: 20, bottom: 30};

const ENTER_KEY = UIManager.RCTView.Constants.Keys.Enter;
const HANDLED_KEYS = [ENTER_KEY];

/**
 * A wrapper for making views respond properly to touches.
 * On press down, the opacity of the wrapped view is decreased, which allows
 * the underlay color to show through, darkening or tinting the view.
 *
 * The underlay comes from wrapping the child in a new View, which can affect
 * layout, and sometimes cause unwanted visual artifacts if not used correctly,
 * for example if the backgroundColor of the wrapped view isn't explicitly set
 * to an opaque color.
 *
 * TouchableButtonWindows must have one child (not zero or more than one).
 * If you wish to have several child components, wrap them in a View.
 *
 * Example:
 *
 * ```
 * renderButton: function() {
 *   return (
 *     <TouchableButtonWindows onPress={this._onPressButton}>
 *       <Image
 *         style={styles.button}
 *         source={require('./myButton.png')}
 *       />
 *     </TouchableButtonWindows>
 *   );
 * },
 * ```
 */

var TouchableButtonWindows = createReactClass({
  displayName: 'TouchableButtonWindows',
  propTypes: {
    ...TouchableWithoutFeedback.propTypes,
    /**
     * Determines what the opacity of the wrapped view should be when touch is
     * active.
     */
    activeOpacity: PropTypes.number,
    /**
     * Determines what the padding of the wrapped view should be when touch is
     * active.
     */
    activePadding: PropTypes.number,
    /**
     * The color of the underlay that will show through when the touch is
     * active.
     */
    underlayColor: ColorPropType,
    style: ViewPropTypes.style,
    /**
     * Called immediately after the underlay is shown
     */
    onShowUnderlay: PropTypes.func,
    /**
     * Called immediately after the underlay is hidden
     */
    onHideUnderlay: PropTypes.func,
   },

  mixins: [NativeMethodsMixin, TimerMixin, Touchable.Mixin],

  getDefaultProps: () => DEFAULT_PROPS,

  // Performance optimization to avoid constantly re-generating these objects.
  _computeSyntheticState: function(props) {
    return {
      activeProps: {
        style: {
          opacity: props.activeOpacity,
        }
      },
      activeUnderlayProps: {
        style: {
          backgroundColor: props.underlayColor,
        }
      },
      focusedProps: {
        style: {
          borderWidth: 2,
          margin: -2,
        }
      },
      underlayStyle: [
        INACTIVE_UNDERLAY_PROPS.style,
        props.style,
      ],
    };
  },

  getInitialState: function() {
    this._isMounted = false;
    return merge(
      this.touchableGetInitialState(), 
      this._computeSyntheticState(this.props), 
      { isFocused: false, isFocusedEnterKeyDown: false },
    );
  },

  componentDidMount: function() {
    this._isMounted = true;
    ensurePositiveDelayProps(this.props);
    ensureComponentIsNative(this.refs[CHILD_REF]);
  },

  componentWillUnmount: function() {
    this._isMounted = false;
  },

  componentDidUpdate: function() {
    ensureComponentIsNative(this.refs[CHILD_REF]);
  },

  componentWillReceiveProps: function(nextProps) {
    ensurePositiveDelayProps(nextProps);
    if (nextProps.activeOpacity !== this.props.activeOpacity ||
        nextProps.underlayColor !== this.props.underlayColor ||
        nextProps.style !== this.props.style) {
      this.setState(this._computeSyntheticState(nextProps));
    }
  },

  viewConfig: {
    uiViewClassName: 'RCTView',
    validAttributes: ReactNativeViewAttributes.RCTView
  },

  /**
   * `Touchable.Mixin` self callbacks. The mixin will invoke these if they are
   * defined on your component.
   */
  touchableHandleActivePressIn: function(e: Event) {
    this.clearTimeout(this._hideTimeout);
    this._hideTimeout = null;
    this._showUnderlay();
    this.props.onPressIn && this.props.onPressIn(e);
  },

  touchableHandleActivePressOut: function(e: Event) {
    if (!this._hideTimeout) {
      this._hideUnderlay();
    }
    this.props.onPressOut && this.props.onPressOut(e);
  },

  touchableHandlePress: function(e: Event) {
    this.clearTimeout(this._hideTimeout);
    this._showUnderlay();
    this._hideTimeout = this.setTimeout(this._hideUnderlay,
      this.props.delayPressOut || 100);
    this.props.onPress && this.props.onPress(e);
  },

  touchableHandleLongPress: function(e: Event) {
    this.props.onLongPress && this.props.onLongPress(e);
  },

  touchableGetPressRectOffset: function() {
    return this.props.pressRetentionOffset || PRESS_RETENTION_OFFSET;
  },

  touchableGetHitSlop: function() {
    return this.props.hitSlop;
  },

  touchableGetHighlightDelayMS: function() {
    return this.props.delayPressIn;
  },

  touchableGetLongPressDelayMS: function() {
    return this.props.delayLongPress;
  },

  touchableGetPressOutDelayMS: function() {
    return this.props.delayPressOut;
  },

  _showUnderlay: function() {
    if (!this._isMounted || !this._hasPressHandler()) {
      return;
    }

    this.refs[UNDERLAY_REF].setNativeProps(this.state.activeUnderlayProps);
    this.refs[CHILD_REF].setNativeProps(this.state.activeProps);
    this.props.onShowUnderlay && this.props.onShowUnderlay();
  },

  _hideUnderlay: function() {
    this.clearTimeout(this._hideTimeout);
    this._hideTimeout = null;
    if (this._hasPressHandler() && this.refs[UNDERLAY_REF]) {
      this.refs[CHILD_REF].setNativeProps(INACTIVE_CHILD_PROPS);
      this.refs[UNDERLAY_REF].setNativeProps({
        ...INACTIVE_UNDERLAY_PROPS,
        style: this.state.underlayStyle,
      });
      this.props.onHideUnderlay && this.props.onHideUnderlay();
    }
  },

  _hasPressHandler: function() {
    return !!(
      this.props.onPress ||
      this.props.onPressIn ||
      this.props.onPressOut ||
      this.props.onLongPress
    );
  },

  _onFocus: function() {
    this.setState({isFocused: true});
  },

  _onBlur: function() {
    this.setState({isFocused: false});      
  },

  _onKeyDown: function(e: Event) {
    if (e.nativeEvent.key === ENTER_KEY) {
      this.touchableHandleActivePressIn(e);
    }
  },

  _onKeyUp: function(e: Event) {
    if (e.nativeEvent.key === ENTER_KEY) {
      this.touchableHandleActivePressOut(e);        
    }
  },

  render: function() {
    let underlayStyle = this.state.underlayStyle;
    if (this.state.isFocused) {
      underlayStyle = [ this.state.focusedProps.style, underlayStyle ];
    }

    return (
      <View
        accessible={this.props.accessible !== false}
        accessibilityLabel={this.props.accessibilityLabel}
        accessibilityComponentType={this.props.accessibilityComponentType}
        accessibilityTraits={this.props.accessibilityTraits}
        ref={UNDERLAY_REF}
        style={underlayStyle}
        onLayout={this.props.onLayout}
        hitSlop={this.props.hitSlop}
        onStartShouldSetResponder={this.touchableHandleStartShouldSetResponder}
        onResponderTerminationRequest={this.touchableHandleResponderTerminationRequest}
        onResponderGrant={this.touchableHandleResponderGrant}
        onResponderMove={this.touchableHandleResponderMove}
        onResponderRelease={this.touchableHandleResponderRelease}
        onResponderTerminate={this.touchableHandleResponderTerminate}
        isTabStop={!this.props.disabled}
        onFocus={this._onFocus}
        onBlur={this._onBlur}
        onKeyDown={this._onKeyDown}
        onKeyUp={this._onKeyUp}
        handledKeyDownKeys={HANDLED_KEYS}
        handledKeyUpKeys={HANDLED_KEYS}
        nativeID={this.props.nativeID}
        testID={this.props.testID}>
        {React.cloneElement(
          React.Children.only(this.props.children),
          {
            ref: CHILD_REF,
          }
        )}
        {Touchable.renderDebugView({color: 'green', hitSlop: this.props.hitSlop})}
      </View>
    );
  }
});

var CHILD_REF = keyOf({childRef: null});
var UNDERLAY_REF = keyOf({underlayRef: null});
var INACTIVE_CHILD_PROPS = {
  style: StyleSheet.create({x: {opacity: 1.0}}).x,
};
var INACTIVE_UNDERLAY_PROPS = {
  style: StyleSheet.create({x: {backgroundColor: 'transparent'}}).x,
};

module.exports = TouchableButtonWindows;
