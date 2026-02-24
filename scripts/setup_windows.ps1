# Requires PowerShell 5+.

$ErrorActionPreference = "Stop"

Write-Host "Windows setup notes:"
Write-Host " - Install Visual Studio 2022 (Desktop development with C++)"
Write-Host " - Install CMake (and optionally Ninja)"
Write-Host " - Install Git for Windows (includes bash for running ./scripts/*.sh)"
Write-Host " - Install LLVM clang-format (optional but recommended)"

Write-Host ""
Write-Host "No automatic installs are performed by this script."
