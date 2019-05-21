// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

using System;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;

using react.uwp;

namespace PerfCompare
{
    public sealed partial class ReactNativeVNextPage : Page
    {
        private IInstance _instance;

        const string JSFILENAME = "index.uwp";
        const string JSCOMPONENTNAME = "PerfCompareRN";

        public ReactNativeVNextPage()
        {
            this.InitializeComponent();
            LoadReact();
        }

        private void LoadReact()
        {
            _instance = Instance.Create(JSFILENAME);

            _instance.Start(new InstanceSettings()
            {
                UseWebDebugger = false,
                UseLiveReload = false,
            });

            RootElement.Instance = _instance;
            RootElement.InitialProps = "{ \"messageCount\": \"" + App.TotalMessages +"\" }";
            RootElement.JsComponentName = JSCOMPONENTNAME;

            RootElement.StartRender();
        }

        private async void Page_Loaded(object sender, RoutedEventArgs e)
        {
            await Dispatcher.RunIdleAsync((args) =>
            {
                App.PerfStats.Stop(false);
                App.ShowStats();
            });
        }
    }
}
