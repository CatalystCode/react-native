'use strict';

import { findNodeHandle, NativeModules } from 'react-native';

/*
** This is a helper class intended to allow usage of Polite/Aggressive Focus for win32.
*/
export default class FocusManager {

    // This function takes in a ref to a React Component and a bool value. If setWindowFocus = true, call aggressive focus.
    // Else, call polite focus
    static focus(ref, setWindowFocus)
    {
        if(ref)
        {
            if(setWindowFocus)
            {
                NativeModules.UIManager.dispatchViewManagerCommand(
                    findNodeHandle(ref),
                    NativeModules.UIManager.getViewManagerConfig('RCTView').Commands.aggressivefocus,
                    null
                );
            }
            else 
            {
                NativeModules.UIManager.dispatchViewManagerCommand(
                    findNodeHandle(ref),
                    NativeModules.UIManager.getViewManagerConfig('RCTView').Commands.politefocus,
                    null
                );
            }
        }
    }
}

module.exports = FocusManager;
