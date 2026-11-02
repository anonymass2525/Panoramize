# Panoramize 5.3 — Windows validation gate + inverse identifiability diagnostics

## 目的

- **本版は万能物理構造復元器の完成版ではない。** 逆問題の観測不足を診断し、追加の音響・物理観測を選ぶ `observability53` を追加。
- Windows x64 VST3の**実ビルド・pluginvalを実施するための** CI/PowerShellスクリプトを更新し、host automation（g48/g49/g51）・MIDI・状態保存復元をJUCEスモークテストに追加。
- **この開発環境自体ではWindows/MSVC、Studio Oneを実行していない**。以下は実行するためのコマンドであって合格記録ではない。

## 新機能と制約

`include/ExperimentalObservability53.h` と `src/ExperimentalObservability53.cpp` は、モデルパラメーターに対する観測量の**対数感度Jacobian**を計算し、測定誤差で正規化した特異値から局所ランクを推定する。測定候補を追加したとき、局所ランクを増やせる観測を探索できる。これは多変数・未知形状・声道の**局所的な同定可能性検査**の部品で、実録音から任意の形状や材質を特定する完成アルゴリズムではない。有限差分・局所計算であり、大域的な識別可能性は証明しない。

試験例: 弦の基本周波数だけから張力Tと線密度μを同定しようとすると1/2ランク。同じ物理比に依存する倍音や別音高を追加しても1/2ランク。線密度や張力の**独立測定**を追加すると2/2ランク。声道の4分の1波長共鳴でも音速と長さの混同を検査。報告値の音響精度ではなく、数学的な観測設計診断である。

## Windows x64 実行手順（必ずWindowsで実行）

1. Visual Studio 2022 C++とCMakeを導入する。
2. JUCE `8.0.16-security` を公式リポジトリから取得する: https://github.com/juce-framework/JUCE/releases/tag/8.0.16-security
3. pluginval `v1.0.4` Windows版を取得して展開する: https://github.com/Tracktion/pluginval/releases/tag/v1.0.4
4. リポジトリ直下のPowerShellで実行:

```powershell
./ci/Build-Test-Windows53.ps1 -JucePath "C:\dev\JUCE" -PluginvalExe "C:\tools\pluginval.exe"
```

GitHub Actionsへ本ソースのリポジトリをpushした場合は `.github/workflows/windows-vst3.yml` を使用。**現環境からそのワークフローを実行したという意味ではない**。成功した場合は`.vst3`、pluginvalログ、`windows_validation53.json`を取得する。失敗した場合は失敗ログを保存する。

5. 実機のStudio Oneで `ci/Windows-StudioOne-Checklist53_JA.md` をすべて実施し、WAV、プロジェクト、ログを記録する。自動スモークが通ってもStudio Oneでの挙動は保証されない。

## Windows / DAW未検証事項

- MSVC JUCE GUIの実際のコンパイルとVST3 DLL生成
- pluginval strictness5 GUIを含む実行結果
- Studio Oneでの多インスタンス、音切れ、automation、MIDI、保存復元、offline bounce
- 実録音による3D形状・材質・声道の未知パラメーター再現性

## VST3互換性

JUCE表示名を5.3に変更。旧5.2バンドルID・`PnGP` / `Pz52`を変更せず、DAWの既存プロジェクトからの参照互換を優先。最終的な互換性もWindowsホストで確認する必要がある。
