﻿using ReactNative.UIManager;
using ReactNative.UIManager.Events;
using System;
using System.Collections.Generic;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Media;
using Windows.UI.Xaml.Media.Imaging;

namespace ReactNative.Views.Image
{
    /// <summary>
    /// The view manager responsible for rendering native <see cref="ImageControl"/>.
    /// TODO: Implememt tintColor property and fadeDuration animation support.
    /// </summary>
    public class ReactImageManager : SimpleViewManager<Border>
    {
        private const string ReactClass = "RCTImageView";
        private const string PROP_SOURCE = "source";
        private const string PROP_URI = "uri";

        /// <summary>
        /// The view manager name.
        /// </summary>
        public override string Name
        {
            get
            {
                return ReactClass;
            }
        }

        /// <summary>
        /// The view manager event constants.
        /// </summary>
        public override IReadOnlyDictionary<string, object> ExportedCustomDirectEventTypeConstants
        {
            get
            {
                return new Dictionary<string, object>
                {
                    { "topLoadEnd", new Dictionary<string, object> { { "registrationName", "onLoadEnd" } } },
                    { "topLoadStart", new Dictionary<string, object> { { "registrationName", "onLoadStart" } } },
                };
            }
        }

        /// <summary>
        /// Sets the <see cref="ImageBrush"/> source for the background of a <see cref="Border"/>.
        /// </summary>
        /// <param name="view">The text input box control.</param>
        /// <param name="degrees">The text alignment.</param>
        [ReactProperty(PROP_SOURCE)]
        public void SetSource(Border view, Dictionary<string, string> sourceMap)
        {
            var imageSrcURL = default(Uri);
            var source = default(string);

            if (sourceMap != null && sourceMap.TryGetValue(PROP_URI, out source))
            {
                if (!Uri.TryCreate(source, UriKind.Absolute, out imageSrcURL))
                {
                    imageSrcURL = new Uri("ms-appx://" + source);
                }

                if (imageSrcURL != null)
                {
                    var backgroundImage = new ImageBrush()
                    {
                        ImageSource = new BitmapImage(imageSrcURL)
                    };

                    view.Background = backgroundImage;
                }
            }
        }

        /// <summary>
        /// The border radius of the <see cref="ReactRootView"/>.
        /// </summary>
        /// <param name="view">The view panel.</param>
        /// <param name="radius">The border radius value.</param>
        [ReactProperty("borderRadius")]
        public void SetBorderRadius(Border view, double radius)
        {
            view.CornerRadius = new CornerRadius(radius);
        }

        /// <summary>
        /// Set the border color of the <see cref="ReactPanel"/>.
        /// </summary>
        /// <param name="view">The view panel.</param>
        /// <param name="index">The property index.</param>
        /// <param name="color">The color hex code.</param>
        [ReactProperty("borderColor", CustomType = "Color")]
        public void SetBorderColor(Border view, uint? color)
        {
            // TODO: what if color is null?
            if (color.HasValue)
            {
                var brush = new SolidColorBrush(ColorHelpers.Parse(color.Value));
                view.BorderBrush = brush;
            }
        }

        /// <summary>
        /// Sets the border thickness of the <see cref="ReactPanel"/>.
        /// </summary>
        /// <param name="view">The view panel.</param>
        /// <param name="index">The property index.</param>
        /// <param name="width">The border width in pixels.</param>
        [ReactPropertyGroup(
            ViewProperties.BorderWidth,
            ViewProperties.BorderLeftWidth,
            ViewProperties.BorderRightWidth,
            ViewProperties.BorderTopWidth,
            ViewProperties.BorderBottomWidth,
            DefaultDouble = double.NaN)]
        public void SetBorderWidth(Border view, int index, double width)
        {
            view.SetBorderWidth(ViewProperties.BorderSpacingTypes[index], width);
        }

        /// <summary>
        /// Creates the image view instance.
        /// </summary>
        /// <param name="reactContext">The react context.</param>
        /// <returns>The image view instance.</returns>
        protected override Border CreateViewInstanceCore(ThemedReactContext reactContext)
        {
            return new Border();
        }

        /// <summary>
        /// Installing the textchanged event emitter on the <see cref="TextInput"/> Control.
        /// </summary>
        /// <param name="reactContext">The react context.</param>
        /// <param name="view">The <see cref="TextBox"/> view instance.</param>
        protected override void AddEventEmitters(ThemedReactContext reactContext, Border view)
        {
            view.Loading += OnInterceptImageLoadingEvent;
            view.Loaded += OnInterceptImageLoadedEvent;
        }

        /// <summary>
        /// Called when the <see cref="Border"/> is detached from view hierarchy and allows for 
        /// additional cleanup by the <see cref="ViewManager{Border}"/>
        /// subclass. Unregister all event handlers for the <see cref="Border"/>.
        /// </summary>
        /// <param name="reactContext">The react context.</param>
        /// <param name="view">The <see cref="Border"/>.</param>
        protected override void OnDropViewInstance(ThemedReactContext reactContext, Border view)
        {
            view.Loaded -= OnInterceptImageLoadedEvent;
            view.Loading -= OnInterceptImageLoadingEvent;
        }

        /// <summary>
        /// The <see cref="Border"/> event interceptor for image load start events for the native control.
        /// </summary>
        /// <param name="sender">The source sender view.</param>
        /// <param name="event">The received event arguments.</param>
        protected void OnInterceptImageLoadingEvent(FrameworkElement sender, object e)
        {
            var border = (Border)sender;
            var imageBrush = GetImageBrush(border);
            if (imageBrush != null)
            {
                var bitmapImage = imageBrush.ImageSource as BitmapImage;
                bitmapImage.DecodePixelHeight = (int)sender.Height;
                bitmapImage.DecodePixelWidth = (int)sender.Width;
                imageBrush.Stretch = Stretch.Fill;
            }

            GetEventDispatcher(border).DispatchEvent(new ReactImageLoadEvent(border.GetTag(), ReactImageLoadEvent.OnLoadStart));
        }

        /// <summary>
        /// The <see cref="Border"/> event interceptor for image load completed events for the native control.
        /// </summary>
        /// <param name="sender">The source sender view.</param>
        /// <param name="event">The received event arguments.</param>
        protected void OnInterceptImageLoadedEvent(object sender, RoutedEventArgs e)
        {
            var senderImage = (Border)sender;
            GetEventDispatcher(senderImage).DispatchEvent(new ReactImageLoadEvent(senderImage.GetTag(), ReactImageLoadEvent.OnLoadEnd));
        }

        private ImageBrush GetImageBrush(Border border)
        {
            return border.Background as ImageBrush;
        }

        private static EventDispatcher GetEventDispatcher(FrameworkElement image)
        {
            return image.GetReactContext().ReactInstance.GetNativeModule<UIManagerModule>().EventDispatcher;
        }
    }
}
