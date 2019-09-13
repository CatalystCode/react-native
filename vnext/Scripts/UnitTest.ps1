# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.
#
# UnitTest.ps1
#
param (
	[ValidateSet('x64', 'x86')]
	[string] $Platform = 'x64',

	[ValidateSet('Debug', 'Release')]
	[string] $Configuration = 'Debug',

	[string[]] $Include,

	[string[]] $Exclude,

	[System.IO.FileInfo[]] $Assemblies =
	(
		"$PSScriptRoot\..\target\$Platform\$Configuration\" +
		"React.Windows.Desktop.UnitTests\React.Windows.Desktop.UnitTests.dll"
	),

	[System.IO.FileInfo] $VsTest = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\2019\Enterprise\Common7\IDE\CommonExtensions\Microsoft\TestWindow\vstest.console.exe"
)

if ($Include.Count) {
	$filter = "(FullyQualifiedName~" + ($Include -join ')&(FullyQualifiedName~') + ")"
}

if ($Exclude.Count) {
	$filter += ('', '&')[$filter.Length -gt 0] + "(FullyQualifiedName!~" + ($Exclude -join ')&(FullyQualifiedName!~') + ")"
}

# Run Unit Test assemblies.
& $VsTest $Assemblies --InIsolation --Platform:$Platform ('', "--TestCaseFilter:$filter")[$filter.Length -gt 0]
