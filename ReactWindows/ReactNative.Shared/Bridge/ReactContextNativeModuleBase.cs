using ReactNative.Bridge.Queue;
using System;

namespace ReactNative.Bridge
{
    /// <summary>
    /// Base class for React Native modules that require access to the 
    /// <see cref="ReactContext"/>.
    /// </summary>
    public abstract class ReactContextNativeModuleBase : NativeModuleBase
    {
        /// <summary>
        /// Instantiates the <see cref="ReactContextNativeModuleBase"/>.
        /// </summary>
        /// <param name="reactContext">The React context.</param>
        protected ReactContextNativeModuleBase(IReactContext reactContext)
        {
            if (reactContext == null)
                throw new ArgumentNullException(nameof(reactContext));

            Context = reactContext;
        }

        /// <summary>
        /// Instantiates the <see cref="ReactContextNativeModuleBase"/>.
        /// </summary>
        /// <param name="reactContext">The React context.</param>
        /// <param name="actionQueue">The action queue.</param>
        protected ReactContextNativeModuleBase(IReactContext reactContext, IActionQueue actionQueue)
            : base(actionQueue)
        {
            if (reactContext == null)
                throw new ArgumentNullException(nameof(reactContext));

            Context = reactContext;
        }

        /// <summary>
        /// Instantiates the <see cref="ReactContextNativeModuleBase"/>.
        /// </summary>
        /// <param name="reactContext">The React context.</param>
        /// <param name="delegateFactory">The React method delegate factory.</param>
        protected ReactContextNativeModuleBase(IReactContext reactContext, IReactDelegateFactory delegateFactory)
            : base(delegateFactory)
        {
            if (reactContext == null)
                throw new ArgumentNullException(nameof(reactContext));

            Context = reactContext;
        }

        /// <summary>
        /// Instantiates the <see cref="ReactContextNativeModuleBase"/>.
        /// </summary>
        /// <param name="reactContext">The React context.</param>
        /// <param name="delegateFactory">The React method delegate factory.</param>
        /// <param name="actionQueue">The action queue.</param>
        protected ReactContextNativeModuleBase(IReactContext reactContext, IReactDelegateFactory delegateFactory, IActionQueue actionQueue)
            : base(delegateFactory, actionQueue)
        {
            if (reactContext == null)
                throw new ArgumentNullException(nameof(reactContext));

            Context = reactContext;
        }

        /// <summary>
        /// The React context.
        /// </summary>
        public IReactContext Context { get; }
    }
}
