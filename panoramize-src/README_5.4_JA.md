# Panoramize 5.4 — inverse decision + Windows handoff

## 現実の達成範囲

- 5.0の複数録音・複数既存DSP候補を**元の編集可能な`.pgraph`のまま保持**し、`candidate_uncertainty54.csv`、`decision54.txt`に学習／未使用録音の順位逆転・過適合の疑い・上位候補の近接度を記録。
- `next_measurements54.csv`には、弦の張力・線密度と管の音速・長さの**2変数の解析的観測モデル**に対し、追加の独立測定が局所ランクを増やすかを出力。**任意3D FEMや人声の内部状態に対する完全な測定設計ではない**。
- これまでの`panoramize_joint50`の`--demo`／`--infer-provenance`／`--infer-manifest`で上記追加レポートを自動出力。
- Windows VST3をビルドしてpluginval厳格度5まで検証する `ci/Build-Test-Windows54.ps1` を追加。**Windows/MSVC/Studio Oneの実際の実行はWindows機でのみ可能**。
- VST3のプラグイン識別子`PnGP/Pz52`は維持。古いセッションの互換性はDAW実機で検証が必要。

## Windows PCで実ビルドする方法

Visual Studio 2022 に「C++によるデスクトップ開発」とWindows SDK、CMake、Git、PowerShellを導入してください。

PowerShellでZIPの展開先（`CMakeLists.txt`のあるディレクトリ）へ移動して:

```powershell
powershell -ExecutionPolicy Bypass -File .\ci\Build-Test-Windows54.ps1 -InstallScope User
```

上記はJUCE 8.0.16-securityとpluginval 1.0.4を公式GitHubから自動取得します。MSVC Releaseビルド、選択したCTest、pluginval strictness 5の成功後に、ユーザーVST3フォルダーへインストールします。既存プラグインがある場合は名前を変更してバックアップします。インストールなしの検証は`-InstallScope None`。

- 公式JUCE: https://github.com/juce-framework/JUCE/releases/tag/8.0.16-security
- 公式pluginval: https://github.com/Tracktion/pluginval/releases/tag/v1.0.4
- PreSonus Studio Oneスキャン説明: https://support.presonus.com/hc/en-us/articles/360045544271-How-can-I-get-my-3rd-party-plug-ins-to-show-up-in-Studio-One

既定のVST3導入先は`C:\Program Files\Common Files\VST3`、新しいStudio Oneではユーザー単位の`%LOCALAPPDATA%\Programs\Common\VST3`も読み込み対象です。プラグインが表示されなければStudio OneのVSTプラグインのスキャン設定を確認してください。システム共通フォルダーへの導入は管理者PowerShellで `-InstallScope Machine` を指定します。

合否は`windows-validation54/acceptance54.json` と各ログで確認します。`studio_one_manually_verified`は**スクリプトが絶対にtrueにしません**。Studio Oneのロード／音声／保存復元／オートメーション確認は人の操作が必要です。

## Studio One実機受け入れチェック

1. Studio Oneを終了してからVST3を配置。起動してインストゥルメントとしてロード（必要なら再スキャン）。
2. 48 kHz／128サンプルでC4のノートを発音、ノートオフ、複数ノート、サステインを試す。
3. GUIを開閉し、マクロ・カットオフのホストオートメーションを記録して動くか確認。
4. その状態でプロジェクトを保存、Studio Oneを完全に終了、再起動し、値・音色・MIDI演奏を照合。
5. 48 kHz／64・128・256サンプルで音切れ／ノイズを調べる。リアルタイム／オフライン書き出しも比較する。
6. クラッシュ・黒いGUI・無音・オートメーション不一致があれば `windows-validation54` フォルダーのログと再現条件を保存。

**警告**: 初回ビルドではWindows側のJUCE API差分・プラグインの不具合が発見される可能性があります。Linux CTestの合格はWindowsコンパイルの保証ではありません。既存の重要なDAWプロジェクトではなく新規の空プロジェクトで試してください。
