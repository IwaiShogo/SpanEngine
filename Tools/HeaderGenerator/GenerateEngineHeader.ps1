# 設定
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$RootDir = (Get-Item $ScriptDir).Parent.Parent.FullName
$SourceDir = Join-Path $RootDir "Engine\Source"
$OutputFile = Join-Path $SourceDir "SpanEngine.h"

# ターゲットディレクトリ
$TargetDirs = @("Core", "Runtime")

Write-Host "Generating SpanEngine.h..."

# ヘッダーリストの作成
$Includes = @()
foreach ($Target in $TargetDirs) {
	$TargetDir = Join-Path $SourceDir $Target
	if (Test-Path $TargetDir) {
		$Files = Get-ChildItem -Path $TargetDir -Recurse -Filter "*.h"
		foreach ($File in $Files) {
			# 相対パスを取得してスラッシュに置換
			$RelPath = $File.FullName.Substring($SourceDir.Length + 1).Replace("\", "/")

			# EntryPoint.h は main関数を持つため、一括インクルードからは除外する
			if ($RelPath -match "EntryPoint.h") {
				continue
			}

			$Includes += "#include `"$RelPath`""
		}
	}
}

# ソート
$Includes = $Includes | Sort-Object

# ファイル内容の構築
$Content = @"
#pragma once
// ==========================================
// AUTO-GENERATED FILE. DO NOT MODIFY.
// This file includes all Core and Runtime headers.
// ==========================================

// Core & Runtime
$($Includes -join "`n")
"@

# 既存ファイルと比較して、変更がある場合のみ書き込む (リビルド回避)
$NeedUpdate = $true
if (Test-Path $OutputFile) {
	$CurrentContent = Get-Content $OutputFile -Raw -Encoding UTF8
	# 改行コードの違いを無視して比較するためにTrim
	if ($CurrentContent.Trim() -eq $Content.Trim()) {
		$NeedUpdate = $false
	}
}

if ($NeedUpdate) {
	Set-Content -Path $OutputFile -Value $Content -Encoding UTF8
	Write-Host "Updated: $OutputFile"
} else {
	Write-Host "Skipped: No changes detected."
}
