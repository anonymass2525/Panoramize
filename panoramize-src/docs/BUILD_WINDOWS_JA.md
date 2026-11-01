# Panoramize 2.7.0 — Windows x64 / Studio One向け VST3 ビルド

**現時点でWindows VST3のビルド・pluginval・Studio Oneの実動作は未検証です。**
LinuxのCTestやASanの成功は、WindowsでのGUIとVST3の動作保証ではありません。

## GitHub Actions (推奨)

1. 本ソースZIPをGitHubリポジトリのルートに展開してコミット。
2. `.github/workflows/windows-vst3.yml` を利用して、Actionsでワークフローを実行。
3. Windows x64 Releaseビルド → CTest（JUCE Processor/Editorを含む） → VST3構造検証 → pluginval strictness 5がすべて成功した場合のみ、成果物 `Panoramize-v2.7.0-Windows-x64-VST3-validated` が保存されます。
4. 成功したVST3バンドルをWindows標準の `C:\Program Files\Common Files\VST3\` に配置し、Studio Oneで再スキャン。

## Windows PowerShell（ローカル）

依存: Windows 10/11 x64、Visual Studio 2022 C++ Desktop、CMake、JUCE 8.0.16-security。

```powershell
git clone --depth 1 --branch 8.0.16-security https://github.com/juce-framework/JUCE.git C:\SDK\JUCE
powershell -ExecutionPolicy Bypass -File .\ci\Build-WindowsVST3.ps1 `
  -JucePath "C:\SDK\JUCE" -PluginvalExe "C:\SDK\pluginval\pluginval.exe"
```

pluginvalがない場合は -PluginvalExe を省略できますが、pluginvalの合格とはみなせません。
配布ファイルにはJUCEのソースは含めず、ライセンス確認が必要です。

公式: https://github.com/juce-framework/JUCE/releases/tag/8.0.16-security
公式: https://github.com/Tracktion/pluginval/releases/tag/v1.0.4

## 検証条件

参照: `docs/STUDIO_ONE_V27_ACCEPTANCE_JA.md`。Windows VST3実行とStudio Oneテストは今後の実機で検証する項目です。
