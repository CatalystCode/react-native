// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#include <Modules/ThemingModule.h>

#include <winrt/Windows.UI.ViewManagement.h>

#include <cxxreact/MessageQueueThread.h>

#include <IReactInstance.h>

namespace react {
  namespace uwp {

    class Theming : public react::windows::PlatformTheme
    {
    private:
      enum ThemingEvent
      {
        Theme = 0,
        HighContrast = 1
      };

    public:
      Theming(const std::shared_ptr<IReactInstance>& reactInstance, const std::shared_ptr<facebook::react::MessageQueueThread>& defaultQueueThread);
      virtual ~Theming();

      const std::string getTheme() override;
      bool getIsHighContrast() override;

    private:
      void fireEvent(ThemingEvent event, std::string const& args);

      std::weak_ptr<IReactInstance> m_wkReactInstance;
      std::shared_ptr<facebook::react::MessageQueueThread> m_queueThread;
      winrt::Windows::UI::Xaml::ApplicationTheme m_currentTheme{ winrt::Windows::UI::Xaml::ApplicationTheme::Light };
      bool m_isHighContrast;

      winrt::Windows::UI::ViewManagement::AccessibilitySettings m_accessibilitySettings{ };
      winrt::Windows::UI::ViewManagement::AccessibilitySettings::HighContrastChanged_revoker m_highContrastChangedRevoker{ };
      winrt::Windows::UI::ViewManagement::UISettings m_uiSettings{ };
      winrt::Windows::UI::ViewManagement::UISettings::ColorValuesChanged_revoker m_colorValuesChangedRevoker{ };
    };

  }
} // namespace react::uwp
