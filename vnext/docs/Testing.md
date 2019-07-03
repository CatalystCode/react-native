# Testing ReactNative for Windows

When contributing to this project, unit and integration tests should be run to help prevent new bugs and regressions.

## Windows Desktop
Tests should be run with a VSTest-compatible client
(i.e. [VSTest.Console.exe](https://docs.microsoft.com/en-us/visualstudio/test/vstest-console-options?view=vs-2019),
[Visual Studio Test task](https://docs.microsoft.com/en-us/azure/devops/pipelines/tasks/test/vstest?view=azure-devops)).<br/>
This project includes convenience scripts to set up and run the test artifacts.

### Unit Tests
*Implemented in the React.Windows.Desktop.UnitTests project.*<br/>
Isolated, self-contained tests covering concrete implementations against mock dependencies and data.
```powershell
& Scripts\UnitTests.ps1
```

### Integration Tests
*Implemented in the React.Windows.Desktop.IntegrationTests project.*<br/>
Higher-level workflows interacting with real external dependencies (i.e. file system, networking servers).<br/>
These tests should can still run on the local development host.
```powershell
& Scripts\IntegrationTests -Setup # Sets and starts up runtime dependencies.
& Scripts\IntegrationTests.ps1
```
