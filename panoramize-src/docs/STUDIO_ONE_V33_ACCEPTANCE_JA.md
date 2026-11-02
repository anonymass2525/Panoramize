# Panoramize 3.3.0 — Studio One導入・演奏の受け入れ検証

本バージョンのWindows VST3／JUCE GUI／pluginval／Studio One実機は未検証です。以下は**実機で実行する確認項目**で、成功済みの記録ではありません。

1. Windows GitHub Actions `.github/workflows/windows-vst3.yml` のReleaseビルドとDSP+JUCE Smoke、pluginval strictness 5まで成功するか確認。
2. 成功したVST3 artifactをインストール。旧版は削除せず、製品コード `Pz33` の新プラグインとして読み込む。
3. Universal Patchbayの **NEW PHYSICAL 3.3** からMIDI C3〜C5が鳴るか確認。
4. **EDIT ARTICULATION** を開き、Hammer／Pull／Slide／Palm Damping／Palm Positionの設定を変更し、保存・再読み込みできるか確認。
5. **CC20=48**を送って重複するNote On（押弦上昇）を実行。ハンマリングによる新しい弦励振が生じるか確認。
6. **CC20=80**を送って低い音へのNote Onを重ね、プリング音を確認。
7. **CC20=110**を送り、移動先ノートへスライド。音高変化と指板摩擦成分を確認。
8. **CC22**でブリッジ側の接触深さを操作、**CC23**で接触位置を動かし、倍音と減衰が変化するか確認。
9. Pitch Bend、Channel Pressure、Poly Pressure、Sustain CC64、無音停止、録音・オフライン書き出し・保存復元を確認。
10. 44.1/48/96kHz、128/256/512 sample block、1/4/8/16音、複数インスタンス、長時間・再生停止・サンプルレート変更に対するクラッシュ・無音・ノイズを検証。

`CC20` は奏法を**次の重複Note Onに**指定します。CC20値0–31通常レガート／32–63ハンマリング／64–95プリング／96–127スライドです。CC21は強度、CC22は接触深さ、CC23は接触位置です。`CC21` を使う場合はCC20の後で送ってください。

旧`.pgraph`の形式保持をLinux DSPでは確認済み。ただし旧版のセッションを新しい`Pz33`へ自動的に差し替えることはできません。もしWindowsビルドやプラグインのUIに問題があれば、エラーログを優先して修正してください。
