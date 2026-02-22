#!/usr/bin/env pwsh
# RoboPartner Installation Script for Windows
# Supports: Windows 10, Windows 11

#Requires -Version 7.0

# Error handling preference
$ErrorActionPreference = "Stop"

# Script parameters
[CmdletBinding(SupportsShouldProcess = $true)]
param(
    [string]$InstallDir = "$env:USERPROFILE\.robopartner",
    [string]$RepoUrl = "",
    [string]$CommitHash = "",
    [switch]$SkipPathCheck = $false
)

# Color output functions
function Write-Info {
    param([string]$Message)
    Write-Host ">>> $Message" -ForegroundColor Cyan
}

function Write-Success {
    param([string]$Message)
    Write-Host ">>> $Message" -ForegroundColor Green
}

function Write-Warning {
    param([string]$Message)
    Write-Host ">>> $Message" -ForegroundColor Yellow
}

function Write-Error {
    param([string]$Message)
    Write-Host ">>> $Message" -ForegroundColor Red
}

# Validate Git repository URL for security
function Test-GitUrl {
    param([string]$Url)

    if ([string]::IsNullOrWhiteSpace($Url)) {
        return $false
    }

    # Check if URL starts with https:// or git://
    if (-not ($Url -match '^https://|^git://')) {
        Write-Error "Invalid repository URL: must start with https:// or git://"
        return $false
    }

    # Check if URL ends with .git
    if (-not ($Url -match '\.git$')) {
        Write-Error "Invalid repository URL: must end with .git"
        return $false
    }

    # Check for suspicious characters that could lead to command injection
    if ($Url -match '[;&|`$]') {
        Write-Error "Invalid repository URL: contains suspicious characters"
        return $false
    }

    return $true
}

# Validate installation directory path for security
function Test-InstallDir {
    param([string]$Path)

    try {
        # Resolve the path to get absolute path
        $resolvedPath = Resolve-Path $Path -ErrorAction SilentlyContinue

        if (-not $resolvedPath) {
            # Path doesn't exist yet, check parent directory
            $parentDir = Split-Path $Path -Parent
            if ($parentDir) {
                $resolvedParent = Resolve-Path $parentDir -ErrorAction SilentlyContinue
                if (-not $resolvedParent) {
                    Write-Error "Parent directory does not exist: $parentDir"
                    return $false
                }
            } else {
                Write-Error "Invalid install directory path"
                return $false
            }
        }

        # Warn if installing outside user profile
        $userProfile = Resolve-Path $env:USERPROFILE
        if ($resolvedPath -and $resolvedPath.Path -notlike "$userProfile*") {
            Write-Warning "Installing outside user profile directory: $resolvedPath"
            Write-Warning "This may require administrator privileges"
            $response = Read-Host "Continue? (y/N)"
            if ($response -ne 'y' -and $response -ne 'Y') {
                return $false
            }
        }

        return $true
    } catch {
        Write-Error "Failed to validate install directory: $_"
        return $false
    }
}

# Detect Windows version
function Test-WindowsVersion {
    $version = [Environment]::OSVersion.Version
    Write-Info "检测到系统: Windows $($version.Major).$($version.Minor)"
    Write-Info "Detected system: Windows $($version.Major).$($version.Minor)"

    if ($version.Major -lt 10) {
        Write-Error "不支持的 Windows 版本"
        Write-Error "Unsupported Windows version"
        return $false
    }
    return $true
}

# Check if a command exists
function Test-Dependency {
    param([string]$Command)
    try {
        $null = Get-Command $Command -ErrorAction Stop
        return $true
    } catch {
        return $false
    }
}

# Check dependencies
function Test-Dependencies {
    $missing = @()

    Write-Info "检查依赖... / Checking dependencies..."

    # Check for CMake
    if (-not (Test-Dependency "cmake")) {
        $missing += "cmake"
    }

    # Check for Visual Studio Build Tools (MSBuild)
    if (-not (Test-Dependency "msbuild")) {
        # Also check for devenv (Visual Studio)
        if (-not (Test-Dependency "devenv")) {
            $missing += "Visual Studio Build Tools (MSBuild)"
        }
    }

    # Check for git (needed for cloning)
    if (-not (Test-Dependency "git")) {
        $missing += "git"
    }

    if ($missing.Count -gt 0) {
        Write-Error "缺少依赖: $($missing -join ', ')"
        Write-Error "Missing dependencies: $($missing -join ', ')"
        Write-Warning ""
        Write-Warning "安装方法 / Installation instructions:"
        Write-Warning "  - CMake: https://cmake.org/download/ or 'choco install cmake'"
        Write-Warning "  - Visual Studio Build Tools: https://visualstudio.microsoft.com/downloads/"
        Write-Warning "  - Git: https://git-scm.com/download/win or 'choco install git'"
        return $false
    }

    Write-Success "依赖检查通过 / Dependencies satisfied"
    return $true
}

# Detect Visual Studio generator
function Get-CMakeGenerator {
    $generators = @(
        "Visual Studio 17 2022",
        "Visual Studio 16 2019",
        "Visual Studio 15 2017"
    )

    foreach ($gen in $generators) {
        # Check if the corresponding Visual Studio version is installed
        if ($gen -match "Visual Studio (\d+) (\d+)") {
            $vsYear = $matches[2]
            $vsPath = "${env:ProgramFiles}\Microsoft Visual Studio\$vsYear\Community"
            if (-not (Test-Path $vsPath)) {
                $vsPath = "${env:ProgramFiles}\Microsoft Visual Studio\$vsYear\Professional"
            }
            if (-not (Test-Path $vsPath)) {
                $vsPath = "${env:ProgramFiles}\Microsoft Visual Studio\$vsYear\Enterprise"
            }
            if (-not (Test-Path $vsPath)) {
                $vsPath = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\$vsYear\BuildTools"
            }

            if (Test-Path $vsPath) {
                Write-Info "使用 CMake 生成器: $gen"
                Write-Info "Using CMake generator: $gen"
                return $gen
            }
        }
    }

    # Fallback - let CMake decide
    Write-Warning "未检测到 Visual Studio，使用默认生成器"
    Write-Warning "Visual Studio not detected, using default generator"
    return ""
}

# Get source directory (local or download)
function Get-SourceDirectory {
    # Check if we're in a git repository
    $isGitRepo = $false
    try {
        $null = git rev-parse --git-dir 2>$null
        $isGitRepo = $true
    } catch {
        $isGitRepo = $false
    }

    if ($isGitRepo) {
        Write-Info "检测到本地仓库，直接使用..."
        Write-Info "Detected local repository, using current directory..."
        return Get-Location
    }

    # Download from repository
    if ([string]::IsNullOrWhiteSpace($RepoUrl)) {
        Write-Warning "REPO_URL 未设置，请手动指定或从本地目录运行"
        Write-Warning "REPO_URL not set, please specify manually or run from local directory"
        Write-Info "使用方法: .\install.ps1 -RepoUrl <your-repo-url>"
        Write-Info "Usage: .\install.ps1 -RepoUrl <your-repo-url>"
        throw "REPO_URL not configured"
    }

    # Validate repository URL for security
    if (-not (Test-GitUrl -Url $RepoUrl)) {
        throw "Invalid repository URL"
    }

    $tempDir = Join-Path $env:TEMP "robopartner-build"

    try {
        if (Test-Path $tempDir) {
            Remove-Item -Recurse -Force $tempDir -ErrorAction SilentlyContinue
        }
        New-Item -ItemType Directory -Force -Path $tempDir | Out-Null

        Write-Info "正在下载源码... / Downloading source code..."
        Write-Info "Repository: $RepoUrl"

        Push-Location $tempDir
        git clone $RepoUrl robopartner 2>$null
        if ($LASTEXITCODE -ne 0) {
            throw "git clone failed"
        }
        Set-Location robopartner

        # Verify commit hash if provided
        if ($CommitHash) {
            Write-Info "验证提交哈希: $CommitHash / Verifying commit hash..."
            $currentHash = git rev-parse HEAD 2>$null
            if ($currentHash -ne $CommitHash) {
                Write-Warning "当前提交哈希: $currentHash"
                Write-Warning "Expected commit hash: $CommitHash"
                Write-Warning "提交哈希不匹配，尝试切换..."
                Write-Warning "Commit hash mismatch, attempting to checkout..."

                git checkout $CommitHash 2>$null
                if ($LASTEXITCODE -ne 0) {
                    throw "Failed to verify commit hash"
                }
            }
            Write-Success "提交哈希验证通过 / Commit hash verified"
        }

        Pop-Location

        Write-Success "源码下载完成 / Source code downloaded"
        return Join-Path $tempDir "robopartner"
    } catch {
        Pop-Location -ErrorAction SilentlyContinue
        Write-Error "无法下载源码: $_"
        Write-Error "Failed to download source code: $_"
        throw
    }
}

# Build RoboPartner
function Build-RoboPartner {
    param([string]$SourceDir)

    $buildDir = Join-Path $SourceDir "build"

    try {
        # Create build directory
        if (Test-Path $buildDir) {
            Remove-Item -Recurse -Force $buildDir -ErrorAction SilentlyContinue
        }
        New-Item -ItemType Directory -Force -Path $buildDir | Out-Null

        Write-Info "正在配置 CMake... / Configuring CMake..."
        Push-Location $buildDir

        $generator = Get-CMakeGenerator
        $cmakeArgs = @("..", "-DCMAKE_BUILD_TYPE=Release")
        if ($generator) {
            $cmakeArgs += @("-G", $generator)
        }

        Write-Info "cmake $($cmakeArgs -join ' ')"
        & cmake $cmakeArgs

        if ($LASTEXITCODE -ne 0) {
            throw "CMake 配置失败 / CMake configuration failed"
        }

        Write-Info "正在编译... / Compiling..."
        # Build with limited parallelism to avoid resource exhaustion
        $maxJobs = [Math]::Min(4, [Environment]::ProcessorCount)
        & cmake --build . --config Release --parallel $maxJobs

        if ($LASTEXITCODE -ne 0) {
            throw "编译失败 / Build failed"
        }

        Pop-Location

        # Verify build succeeded
        $exePaths = @(
            (Join-Path $buildDir "Release\robopartner.exe"),
            (Join-Path $buildDir "Debug\robopartner.exe"),
            (Join-Path $buildDir "robopartner.exe"),
            (Join-Path $SourceDir "robopartner.exe")
        )

        $exePath = $null
        foreach ($path in $exePaths) {
            if (Test-Path $path) {
                $exePath = $path
                break
            }
        }

        if (-not $exePath) {
            throw "编译失败，找不到可执行文件 / Build failed, executable not found"
        }

        Write-Success "编译成功 / Build successful"
        return $exePath

    } catch {
        Pop-Location -ErrorAction SilentlyContinue
        throw
    }
}

# Install RoboPartner
function Install-RoboPartner {
    param([string]$ExecutablePath)

    try {
        # Create install directory
        if (-not (Test-Path $InstallDir)) {
            New-Item -ItemType Directory -Force -Path $InstallDir | Out-Null
        }

        Write-Info "正在安装到 $InstallDir ..."
        Write-Info "Installing to $InstallDir ..."

        # Copy executable
        $destPath = Join-Path $InstallDir "robopartner.exe"
        Copy-Item $ExecutablePath -Destination $destPath -Force

        Write-Success "安装完成 / Installation complete"
        return $destPath

    } catch {
        Write-Error "安装失败: $_"
        Write-Error "Installation failed: $_"
        throw
    }
}

# Setup PATH
function Setup-Path {
    $binDir = Join-Path $env:USERPROFILE "bin"

    # Create bin directory
    if (-not (Test-Path $binDir)) {
        New-Item -ItemType Directory -Force -Path $binDir | Out-Null
    }

    # Create symlink/shortcut in bin directory
    $exePath = Join-Path $InstallDir "robopartner.exe"
    $linkPath = Join-Path $binDir "robopartner.exe"

    try {
        # Remove existing link if present
        if (Test-Path $linkPath) {
            Remove-Item $linkPath -Force -ErrorAction SilentlyContinue
        }

        # Create a symbolic link (requires admin) or copy the file
        try {
            New-Item -ItemType SymbolicLink -Path $linkPath -Target $exePath | Out-Null
            Write-Info "已创建符号链接 / Symbolic link created"
        } catch {
            # Fallback: copy the file
            Copy-Item $exePath -Destination $linkPath -Force
            Write-Info "已复制文件到 bin 目录 / File copied to bin directory"
        }
    } catch {
        Write-Warning "无法创建链接: $_"
        Write-Warning "Failed to create link: $_"
    }

    # Check if bin directory is in PATH
    $currentPath = [Environment]::GetEnvironmentVariable("Path", "User")
    $pathEntries = $currentPath -split ';'

    if ($binDir -notin $pathEntries) {
        Write-Warning ""
        Write-Warning "bin 目录不在 PATH 中"
        Write-Warning "bin directory not in PATH"
        Write-Info ""
        Write-Info "请手动添加到 PATH:"
        Write-Info "Please add to PATH manually:"
        Write-Info "  1. 打开系统环境变量设置 / Open System Environment Variables"
        Write-Info "  2. 编辑用户变量 Path / Edit user variable Path"
        Write-Info "  3. 添加: $binDir"
        Write-Info "     Add: $binDir"
        Write-Info ""
        Write-Info "或运行 PowerShell 命令 (需要管理员权限):"
        Write-Info "Or run PowerShell command (requires admin):"
        Write-Info "  [Environment]::SetEnvironmentVariable('Path', '$currentPath;$binDir', 'User')"
    } else {
        Write-Success "bin 目录已在 PATH 中 / bin directory is already in PATH"
    }
}

# Main installation function
function Start-Installation {
    Write-Info "=========================================="
    Write-Info "   RoboPartner 安装程序"
    Write-Info "   RoboPartner Installer"
    Write-Info "=========================================="
    Write-Info ""

    if (-not (Test-WindowsVersion)) {
        return $false
    }

    if (-not (Test-Dependencies)) {
        return $false
    }

    # Validate install directory for security
    if (-not (Test-InstallDir -Path $InstallDir)) {
        Write-Error "安装目录验证失败 / Install directory validation failed"
        return $false
    }

    # WhatIf mode support
    if ($PSCmdlet.ShouldProcess($InstallDir, "Install RoboPartner")) {
        $sourceDir = $null
        $exePath = $null

        try {
            $sourceDir = Get-SourceDirectory
            $exePath = Build-RoboPartner -SourceDir $sourceDir
            $installedPath = Install-RoboPartner -ExecutablePath $exePath

            Write-Info ""
            Write-Success "安装成功！/ Installation successful!"
            Write-Info ""
            Write-Info "可执行文件位置: $installedPath"
            Write-Info "Executable location: $installedPath"
            Write-Info ""

            if (-not $SkipPathCheck) {
                Setup-Path
            }

            Write-Info ""
            Write-Info "运行命令: robopartner"
            Write-Info "Run command: robopartner"
            Write-Info ""

            return $true

        } catch {
            Write-Error ""
            Write-Error "安装失败: $_"
            Write-Error "Installation failed: $_"
            Write-Error ""
            return $false

        } finally {
            # Clean up temp directory if we created it
            if ($sourceDir -and ($sourceDir -like "*\Temp\robopartner-build*")) {
                try {
                    Write-Info "清理临时文件... / Cleaning up temporary files..."
                    Remove-Item -Recurse -Force $sourceDir -ErrorAction SilentlyContinue
                } catch {
                    Write-Warning "清理临时文件失败 / Failed to clean up temporary files"
                }
            }
        }
    } else {
        Write-Info "WhatIf 模式：跳过实际安装 / WhatIf mode: Skipping actual installation"
        return $true
    }
}

# Script entry point
$mainResult = Start-Installation

if (-not $mainResult) {
    exit 1
}

exit 0
