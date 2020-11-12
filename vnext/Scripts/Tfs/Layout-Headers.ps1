# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

param(
	[string] $SourceRoot = ($PSScriptRoot | Split-Path | Split-Path | Split-Path),
	[string] $TargetRoot = "$SourceRoot\vnext\target",
	[System.IO.DirectoryInfo] $ReactWindowsRoot = "$SourceRoot\vnext",
	[System.IO.DirectoryInfo] $ReactNativeRoot = "$SourceRoot\node_modules\react-native",	
	[string[]] $Extensions = ('h', 'hpp', 'def')
)

[xml]$props = gc $PSScriptRoot\..\..\Directory.Build.props
[string] $FollyVersion = $props.Project.PropertyGroup.FollyVersion;
$FollyVersion = $FollyVersion.Trim() # The extracted FollyVersion contains a space at the end that isn't actually present, issue #6216
[System.IO.DirectoryInfo] $FollyRoot = "$SourceRoot\node_modules\.folly\folly-${FollyVersion}";

Write-Host "Source root: [$SourceRoot]"
Write-Host "Destination root: [$TargetRoot]"
Write-Host "React Native root: [$ReactNativeRoot]"

$patterns = $Extensions| ForEach-Object {"*.$_"}

# ReactCommon headers
Get-ChildItem -Path $ReactNativeRoot\ReactCommon -Name -Recurse -Include $patterns | ForEach-Object { Copy-Item `
	-Path        $ReactNativeRoot\ReactCommon\$_ `
	-Destination (New-Item -ItemType Directory $TargetRoot\inc\$(Split-Path $_) -Force) `
	-Force
}

# Yoga headers
Get-ChildItem -Path $ReactNativeRoot\ReactCommon\yoga\yoga -Name -Recurse -Include $patterns | ForEach-Object { Copy-Item `
	-Path        $ReactNativeRoot\ReactCommon\yoga\yoga\$_ `
	-Destination (New-Item -ItemType Directory $TargetRoot\inc\Yoga\$(Split-Path $_) -Force) `
	-Force
}

# Folly headers
Get-ChildItem -Path $FollyRoot -Name -Recurse -Include $patterns | ForEach-Object { Copy-Item `
	-Path        $FollyRoot\$_ `
	-Destination (New-Item -ItemType Directory $TargetRoot\inc\folly\$(Split-Path $_) -Force) `
	-Force
}

# stubs headers
Get-ChildItem -Path $ReactWindowsRoot\stubs -Name -Recurse -Include $patterns | ForEach-Object { Copy-Item `
	-Path        $ReactWindowsRoot\stubs\$_ `
	-Destination (New-Item -ItemType Directory $TargetRoot\inc\stubs\$(Split-Path $_) -Force) `
	-Force
}

# React.Windows.Core headers
Get-ChildItem -Path $ReactWindowsRoot\Shared -Name -Recurse -Include $patterns | ForEach-Object { Copy-Item `
	-Path        $ReactWindowsRoot\Shared\$_ `
	-Destination (New-Item -ItemType Directory $TargetRoot\inc\Shared\$(Split-Path $_) -Force) `
	-Force
}

# React.Windows.Desktop headers
Get-ChildItem -Path $ReactWindowsRoot\Desktop -Name -Recurse -Include '*.h','*.hpp' | ForEach-Object { Copy-Item `
	-Path        $ReactWindowsRoot\Desktop\$_ `
	-Destination (New-Item -ItemType Directory $TargetRoot\inc\ReactWin32\$(Split-Path $_) -Force) `
	-Force
}
# React.Windows.Desktop DEFs
Get-ChildItem -Path $ReactWindowsRoot\Desktop.DLL -Recurse -Include '*.def' | ForEach-Object { Copy-Item `
	-Path        $_ `
	-Destination (New-Item -ItemType Directory $TargetRoot\inc\ -Force) `
	-Force
}

# React.Windows.Test headers
Get-ChildItem -Path $ReactWindowsRoot\Test -Name -Recurse -Include $patterns | ForEach-Object { Copy-Item `
	-Path        $ReactWindowsRoot\Test\$_ `
	-Destination (New-Item -ItemType Directory $TargetRoot\inc\Test\$(Split-Path $_) -Force) `
	-Force
}

# React.Windows.Test DLL DEF files
Get-ChildItem -Path $ReactWindowsRoot\Desktop.Test.DLL -Name -Recurse -Include $patterns | ForEach-Object { Copy-Item `
	-Path        $ReactWindowsRoot\Desktop.Test.DLL\$_ `
	-Destination (New-Item -ItemType Directory $TargetRoot\inc\$(Split-Path $_) -Force) `
	-Force
}

# include headers
Copy-Item -Force -Recurse -Path $ReactWindowsRoot\include -Destination $TargetRoot\inc

# Microsoft.ReactNative.CXX project
Copy-Item -Force -Recurse -Path $ReactWindowsRoot\Microsoft.ReactNative.Cxx -Destination $TargetRoot\

# Microsoft.ReactNative.CXX project JSI files
Copy-Item -Force -Path $ReactNativeRoot\ReactCommon\jsi\jsi\decorator.h -Destination $TargetRoot\Microsoft.ReactNative.Cxx\jsi\
Copy-Item -Force -Path $ReactNativeRoot\ReactCommon\jsi\jsi\instrumentation.h -Destination $TargetRoot\Microsoft.ReactNative.Cxx\jsi\
Copy-Item -Force -Path $ReactNativeRoot\ReactCommon\jsi\jsi\jsi.cpp -Destination $TargetRoot\Microsoft.ReactNative.Cxx\jsi\
Copy-Item -Force -Path $ReactNativeRoot\ReactCommon\jsi\jsi\jsi.h -Destination $TargetRoot\Microsoft.ReactNative.Cxx\jsi\
Copy-Item -Force -Path $ReactNativeRoot\ReactCommon\jsi\jsi\jsi-inl.h -Destination $TargetRoot\Microsoft.ReactNative.Cxx\jsi\
Copy-Item -Force -Path $ReactNativeRoot\ReactCommon\jsi\jsi\threadsafe.h -Destination $TargetRoot\Microsoft.ReactNative.Cxx\jsi\

# NUSPEC
Copy-Item -Force -Path $ReactWindowsRoot\Scripts\*.nuspec -Destination $TargetRoot


#Copy StripAdditionalPlatformsFromNuspec.ps1 for use by publish task
Copy-Item -Force -Path $ReactWindowsRoot\Scripts\StripAdditionalPlatformsFromNuspec.ps1 -Destination $TargetRoot

# Microsoft.ReactNative.VersionCheck.targets
Copy-Item -Force -Path $ReactWindowsRoot\Scripts\Microsoft.ReactNative.VersionCheck.targets -Destination $TargetRoot

# Microsoft.ReactNative.targets
Copy-Item -Force -Path $ReactWindowsRoot\Scripts\Microsoft.ReactNative.targets -Destination $TargetRoot

# Microsoft.ReactNative.Cxx.targets
Copy-Item -Force -Path $ReactWindowsRoot\Scripts\Microsoft.ReactNative.Cxx.targets -Destination $TargetRoot

# Microsoft.ReactNative.Managed.targets
Copy-Item -Force -Path $ReactWindowsRoot\Scripts\Microsoft.ReactNative.Managed.targets -Destination $TargetRoot

# Microsoft.ReactNative.Managed.CodeGen.targets
Copy-Item -Force -Path $ReactWindowsRoot\Scripts\Microsoft.ReactNative.Managed.CodeGen.targets -Destination $TargetRoot

# Natvis files
Copy-Item -Force -Path $ReactWindowsRoot\Folly\Folly.natvis -Destination (New-Item -ItemType Directory $TargetRoot\natvis -Force)
