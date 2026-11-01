# Panoramize v4.8.0 — Production Bridge & Inverse Research

## 開発の実際の到達点
4.7を保持して、長時間オーディオ解析、録音品質診断、パラメトリック自由曲面（4基底）のFEM形状、材質と表面の小規模同時探索、ガイド付き連続演奏FEM逆解析、および実Native27を使う4.7発音器へのDAWオートメーションを追加した。

**完成を意味しないもの**: 実録音環境一般への頑健性の証明、任意の未知材質・形状の一意な特定、制約なしの自由曲面生成、ノート注釈なしの多声長時間逆解析、全発音エンジンの全内部パラメータのライブホスト操作、Windows Studio One実機での動作検証。

## 実装ファイル
- `include/ProductionInverse48.h`, `src/ProductionInverse48.cpp`: 32秒超～10分までのチャンク単位解析、音量・クリップ・静音率・低音量区間から求める仮のノイズ床推定、4基底の三次元FEMメッシュ曲面変形（退化四面体拒否）、train/holdoutを分離した小規模材質・形状探索、注釈付き連続演奏からノートを抽出する逆設計、パラメーター台帳。
- `tools/production48_cli.cpp`: 16/24bit PCM／float32 WAVを読み込み、CSV解析を出力。複数チャンネルは単純平均してモノラル化。マイク位置・部屋・ステレオ位相差は推定しない。
- `tools/render_automation48.cpp`: 実Native27経路の原音・既存音色処理・新ホスト制御の比較WAV。
- `include/GenuineNative47.h`, `src/GenuineNative47.cpp`: 既存アルゴリズムを変更せず、外部パラメーターターゲットと15ms平滑化、オン／オフ復帰、ホストの4マクロ制御。
- `src/UniversalInstrument23.cpp`: 汎用発音グラフでNative47ホストオートメーションを伝達。
- `juce/PluginProcessor.cpp`: APVTSに4.8の安定ID10個（Enable + 9コントロール）を追加。初期Enable=0。音声ブロック境界で現在値を実発音器に転送。

## ホストパラメーター

| JUCE APVTS ID | 対象 | 反映方法 |
|---|---|---|
| `g48Enable` | Native47ホストオートメーションの明示ON/OFF | ON時のみ |
| `g48Param0` | TPTカットオフ 20～20,000 Hz（対数） | Native実発音器・平滑化 |
| `g48Param1` | 共鳴（0.08～2） | Native実発音器・平滑化 |
| `g48Param2` | ドライブ（0.01～20） | Native実発音器・平滑化 |
| `g48Param3` | 出力（0～4） | Native実発音器・平滑化 |
| `g48Param4` | フィルターミックス（0～1） | Native実発音器・平滑化 |
| `g48Param5`～`8` | 4マクロ | Native実発音器 |

現在は **Native47の9項目のみ** がライブホストオートメーション対応。全発音方式には従来の出力レベルとMIDI CCがあるが、各内部パラメーターの共通ライブ操作は未実装。`parameter_registry.csv` に、ライブ更新可能か、再コンパイルが必要かを区別して出力する。

**DAW実機の検証は未実施**。本版ではStudio One、pluginval、Windowsでビルドできたことは主張しない。

## 逆解析の制約
- `analyzeLong`: 最大10分（実ファイルのバイトサイズと解析時間にも制限）。8秒単位で既存DeepInverse46を再利用。長い音程遷移のチャンク境界を跨いだ連続性は完全には保証しない。
- Recording QC: `noiseFloorRms` は録音の静かな10%区間の実効値であり、録音に真の無音部がなければ正確なノイズ床ではない。推定ダイナミックレンジもSNRではない。
- `buildFreeform`: 四面体を張る既知の基底メッシュを4基底の連続関数でワープする。無制約なNURBS、任意多様体、リメッシュ、接触状態の同時最適化ではない。
- `fitSurfaceMaterial`: 既知の形状・境界の事前情報を必要とし、訓練録音でのみ候補をランク付けする。保留録音は最後に評価するだけ。ゲイン差と±24サンプルのオンセット誤差には許容度があるが、室内インパルス応答と複雑な雑音には未対応。
- `fitAnnotatedPerformance`: 長時間録音を使えるが、**ノートオンセット・音高・ベロシティの注釈が前提**。認識から推定まで全自動ではない。

## 再現手順（Linux研究環境）

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build -j6
ctest --test-dir build --output-on-failure -E 'PANORAMA_DSP|Panoramize_DeepInverse46$'
./build/panoramize_production48 --demo demo48
./build/panoramize_production48 --fit-demo demo48
./build/panoramize_production48 --analyze recording.wav recording_analysis
./build/panoramize_automation48_demo demo48
```

本版の検証では、旧長時間テスト2件を除く40/40件合格、新4.8専用のClang ASan/UBSan検査合格。Win VST3、JUCE GUI、pluginval、Studio Oneの実環境検査は行っていない。
