# Panoramize 3.0.0 — Performance-Aware Physical Inverse Synthesis 検証レポート

検証日: 2026-10-31。Linux C++17 GCC/ClangによるDSPおよびCLI検証。Windows VST3/pluginval/Studio One実機は未検証。

## 達成範囲

- `include/PerformancePhysical30.h` / `src/PerformancePhysical30.cpp`: WAVからフレームごとのF0、信頼度、RMS、正の音量変化を計算。短窓RMSから弾弦オンセット候補、F0の連続遷移、急減衰、ビブラート、レガート／スライド候補を抽出。これらは**音響的手掛かりに基づく低信頼度の候補**であり、奏法名の識別を保証しない。
- 2.9の弦発音モデルに、**3軸の線形共鳴フィルタ**を持つ有効ブリッジ・共鳴胴モデルを追加。探索対象は「弦のみ／弦＋ブリッジ／弦＋胴／弦＋ブリッジ＋胴」の4構造、共鳴周波数3〜5候補、励振・集音の3方向プロファイル。**これは実際の3D FEMメッシュでも、未知形状の逆設計でもない。**
- 複数音高・弾弦位置・減衰が異なる録音を同時評価。学習テイクのみで候補の順位を決め、未使用テイクは評価のみに使用。各テイクの弾弦位置・減衰はまだ自動的に分離最適化していない。
- CLI `panoramize_performance30`: WAV単体のイベント解析、複数WAVマニフェストによる構造探索、合成デモ、`.pgraph`のMIDI演奏。
- `UniversalInstrument23`の新ソース `PhysicalCoupled30`（Kind=12）。`PANORAMIZE-GRAPH-30|1` に物理モデルを記録し、実行時に割り当てのないMIDIリアルタイム発音ソースとして処理。既存の2.3〜2.9ファイル読み込みは維持。
- JUCE Universal Patchbayの発音ソース選択に `COUPLED PHYSICAL / INVERSE 3.0` を追加。詳細な3DモデルGUIは未実装。生成された.pgraphをインポートして再演奏する設計。

## 検証

- CMake Ninja GCC Release: 全ターゲットビルド成功。
- CTest: **19/19 PASS**（既存18項目＋新Performance30）。
- Clang AddressSanitizer / UndefinedBehaviorSanitizer: Performance30の関連全テスト合格。
- サンプル検査: `recording_1.wav`、4候補の再合成WAV、MIDIフレーズWAVを出力。すべて有限値で非ゼロ。
- `.physical30`とStudio One向け `.pgraph` の保存・復元、実際のUniversalInstrument23 MIDI再生、破損モデルの拒否、探索の再現性をテスト。
- Windows MSVC VST3ビルド / pluginval / Studio One: **未実施**。

## 既知合成弦による探索

実験はPanoramize自身が作る理想化弦音（16kHz、1.7秒×4テイク）。3テイクを学習、1テイクをHoldout。弦長に対する弾弦位置 0.20/0.24/0.28/0.22、減衰基礎 2.4/2.8/3.4/2.6 [1/s] に変え、音高も4通りに変えた。参照のブリッジ480Hz・胴270Hz・既定3軸方向。

最大70候補を探索した結果:

|候補|構造|学習特徴量誤差|未使用特徴量誤差|推定ブリッジHz|推定胴Hz|
|---|---|---:|---:|---:|---:|
|1|String+Bridge+Body|0.920849|0.609376|350|230|
|2|String+Body|0.942857|0.384911|—|270|
|3|String+Bridge|0.959855|0.568128|350|—|
|4|StringOnly|1.013067|0.462307|—|—|

構造の最良案は真の「弦＋ブリッジ＋胴」と一致したが、**物理パラメータの正解周波数・3軸方向は復元できなかった**。また未使用テイクでは最良案と他案の順位が入れ替わっており、構造を一意に同定したとは言えない。誤差は対数倍音比による独自特徴量であり聴感一致率ではない。

同じ弾弦位置・減衰を使う自己整合性テストでは真のブリッジ480Hz・胴270Hzと3軸方向を探索範囲内から選び、学習誤差約9.45×10^-7、未使用誤差約8.58×10^-6を得た。ただし同じ生成モデルを用いる閉ループ検証である。

## 音声のみの奏法イベント例

16kHzの2.3秒人工フレーズを用意: 0.10秒で弾弦、1.15秒で約196→247Hzの音高変化、1.60秒から約5Hzのビブラート、1.80秒で急減衰。イベント解析器は0.100秒の発音開始、1.125および1.215秒付近の音高遷移、1.835秒の急減衰候補、1.905秒のビブラート候補を記録。レガート／スライド候補は新規アタックを伴わない音高遷移から推定。明らかな時間窓遅れや重複検出がある。手法はリアルギターで学習・検証した技法分類器ではない。

## 不可欠な限界

1. 実ギター録音での精度・汎化性能は**未検証**。多弦和音、激しい歪み、残響、ピッキングノイズで誤動作の可能性がある。
2. マイク・ピックアップの位置、各弦の固有差、材料密度、ヤング率、張力、物体の3Dメッシュは一意に推定できない。3軸方向は3つの離散プロファイルから選ぶのみ。
3. 正確な奏法名（ハンマリング／プリング／スライド／レガート／ミュート）を音声だけから確定する機能はまだない。重なりを持つ音高変化は候補としてのみ認識。
4. 3.0のブリッジ・胴は低次元の線形共鳴カスケードで、弦・胴を保存的に連成させる3D物理行列ソルバーではない。保存・散逸エネルギーの厳密収支をこの全モデルに保証しない。
5. `.pgraph`には発音モデルを埋め込んでMIDI演奏できるが、GUIから3軸・周波数・演奏解析を編集する専用パネルは未実装。WindowsビルドとStudio Oneホストは未試験。
6. 16kHz WAVによるデモの特徴量最適化は重く、解析処理は**オフライン用途**。楽器のMIDI発音コアとは区別する。

## 実行例

```sh
cmake -S Panoramize_v3.0.0 -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build -j4
ctest --test-dir build --output-on-failure
build/panoramize_performance30 --demo demo
build/panoramize_performance30 guitar_monophonic.wav analysis_out
build/panoramize_performance30 recordings.txt inversion_out
build/panoramize_performance30 inversion_out/candidate_1.pgraph midi.wav
```

`recordings.txt` は `train|clip.wav|196` または `holdout|clip.wav|0` 形式。0はF0を音声から推定する指定。候補探索では弦先験モデルの物理パラメータは既定値であり、その全パラメータを最適化してはいない。
