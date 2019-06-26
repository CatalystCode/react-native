// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

/* tslint:disable */

import React = require('react');
import { Text, TouchableHighlight, View } from 'react-native';
import { Theming } from '../../src/index.uwp';

var currentBackgroundColor = ['blue', 'red', '#E6E6E6', '#7fff00'];
const theming = new Theming();

class AccessibilityBaseExample extends React.Component {
  public render() {
    return (
      <View>
        <Text>The following has accessibilityLabel and accessibilityHint:</Text>
        <View
          style={{width:50, height:50, backgroundColor:'blue'}}
          accessibilityLabel="A blue box"
          accessibilityHint="A hint for the blue box."
        />
        <Text>The following has accessible and accessibilityLabel:</Text>
        <View
          style={{ width: 50, height: 50, backgroundColor: 'red' }}
          accessible={true}
          accessibilityLabel="A hint for the red box."
        />
      </View>
    );
  }
}

class HighContrastExample extends React.Component {
  state = {
    isHighContrast: theming.isHighContrast,
    RGBValues: theming.RGBValues,
    currentTheme: theming.currentTheme
  };

  componentDidMount() {
    theming.addListener('highContrastDidChange', this.highContrastHandler);
    theming.addListener('themeDidChange', this.themeChanged);
  }

  // TODO: Make args props
  highContrastHandler = (args: any) => {
    this.setState({isHighContrast : theming.isHighContrast, 
                  RGBValues : theming.RGBValues});
  };

  themeChanged = (args: any) => {
    this.setState({currentTheme : theming.currentTheme});
  }

  public render() {
    if (this.state.isHighContrast) {
      return (
        <View>
          <Text>The following has HighContrast Event awareness:</Text>
          <View
            style={{width: 150, height: 50, backgroundColor: currentBackgroundColor[2]}}
            accessibilityLabel="A blue box"
            accessibilityHint="A hint for the blue box.">
          <Text>isHighContrast: {this.state.isHighContrast ? 'true' : 'false'}</Text>
          </View>
          <Text>ButtonFaceRGB value: {this.state.RGBValues[0]}</Text>
          <Text>ButtonTextRGB value: {this.state.RGBValues[1]}</Text>
          <Text>GrayTextRGB value: {this.state.RGBValues[2]}</Text>
          <Text>HighlightRGB value: {this.state.RGBValues[3]}</Text>
          <Text>HighlightTextRGB value: {this.state.RGBValues[4]}</Text>
          <Text>HotlightRGB value: {this.state.RGBValues[5]}</Text>
          <Text>WindowRGB value: {this.state.RGBValues[6]}</Text>
          <Text>WindowTextRGB value: {this.state.RGBValues[7]}</Text>
          <View
            style={{ width: 150, height: 50, backgroundColor: currentBackgroundColor[3]}}
            accessible={true}
            accessibilityLabel="A hint for the red box."
          />
        </View>
      );
    } else {
      return (
        <View>
          <Text>The following has HighContrast Event awareness:</Text>
          <View
            style={{width: 150, height: 50, backgroundColor: currentBackgroundColor[0]}}
            accessibilityLabel="A blue box"
            accessibilityHint="A hint for the blue box.">
          <Text>isHighContrast: {this.state.isHighContrast ? 'true' : 'false'}</Text>
          </View>
          <Text>ButtonFaceRGB value: {this.state.RGBValues[0]}</Text>
          <Text>ButtonTextRGB value: {this.state.RGBValues[1]}</Text>
          <Text>GrayTextRGB value: {this.state.RGBValues[2]}</Text>
          <Text>HighlightRGB value: {this.state.RGBValues[3]}</Text>
          <Text>HighlightTextRGB value: {this.state.RGBValues[4]}</Text>
          <Text>HotlightRGB value: {this.state.RGBValues[5]}</Text>
          <Text>WindowRGB value: {this.state.RGBValues[6]}</Text>
          <Text>WindowTextRGB value: {this.state.RGBValues[7]}</Text>
          <View
            style={{ width: 150, height: 50, backgroundColor: currentBackgroundColor[1]}}
            accessible={true}
            accessibilityLabel="A hint for the red box."
          />
        </View>
      );
    }
  }
}

class TouchableExamples extends React.Component<{}, any> {
  public state = {
    pressedCount: 0,
  };

  public render() {
    return (
      <View>
        <Text>The following TouchableHighlight has accessibilityLabel, accessibilityHint, accessibilityRole, toolip:</Text>
        <TouchableHighlight
          style={{width:50, height:50, backgroundColor:'blue'}}
          accessibilityLabel="A blue box"
          accessibilityHint="A hint for the blue box."
          accessibilityRole="button"
          onPress={this.press}
          {...{ tooltip: "a blue tooltip" }}
        >
          <Text>Blue</Text>
        </TouchableHighlight>
        <Text accessibilityLiveRegion="polite" accessibilityLabel={"Pressed " + this.state.pressedCount + "times"}>Pressed {this.state.pressedCount} times</Text>
      </View>
    );
  }

  private press = () => {
    this.setState({pressedCount: this.state.pressedCount+1});
  }
}


export const displayName = (_undefined?: string) => {};
export const title = 'Accessibility';
export const description = 'Usage of accessibility properties.';
export const examples = [
  {
    title: 'Label, Hint',
    render: function(): JSX.Element {
      return <AccessibilityBaseExample />;
    },
  },
  {
    title: 'Touchables',
    render: function(): JSX.Element {
      return <TouchableExamples />;
    },
  },
  {
    title: 'HighContrast',
    render: function(): JSX.Element {
      return <HighContrastExample />;
    },
  }
];
