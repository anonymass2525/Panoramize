# Panoramize v3.4.0 検証レポート

## 判定

**物理接触による弦発音の試作基盤を達成**。現実の6弦ギターの完全3Dモデル、音声からの自動逆推定、Studio Oneでの実機検証は未達。

## Linuxテスト

- Release CMake/Ninja: ビルド成功。
- CTest: **23/23合格**（v3.3の22件に新しいContact34を追加）。
- `Contact34`専用テスト: **Clang ASan + UBSan合格**。Releaseの専用テストには`-UNDEBUG`を追加しassertを有効化した。
- 旧3.3 `.pgraph` の読み込み成功、新3.4 `.pgraph` のシリアライズ往復・発音成功。
- MIDI操作: ノート、局所ハンマー/プルインパルス、接触点移動、位置依存パーム、音高変化を検証。
- 双方向の弦–ブリッジ–3軸共鳴系の音響差が出ることを検証。
- Lossless/free-stringテストは1.67秒後のエネルギー比 **0.999955**（この限定条件のみ）。

## CPUパフォーマンス

Linuxコンテナ、48kHz、2,048フレームの物理発音器のみ。時間予算 = 42.667ms。`demo/benchmark.csv`の1回測定。ハードウェア条件やWindowsに一般化できない。

| 独立した弦の数 | 処理時間 (ms) | バッファ比 |
|---|---:|---:|
| 1 | 3.24 | 0.08 |
| 4 | 14.66 | 0.34 |
| 8 | 31.12 | 0.73 |
| 16 | 64.27 | 1.51 |

16音は時間予算を超過したため**リアルタイム対応を保証しない**。

## 実装詳細

- `include/ContactPhysical34.h` / `src/ContactPhysical34.cpp`: 40内点・動く指・片側指板接触・3軸共鳴体・ブリッジの連成振動。
- `juce/ContactEditor34.h`: 接触・材質に相当する有効パラメーターを編集するJUCEパネル。
- `include/UniversalInstrument23.h`, `src/UniversalInstrument23.cpp`: 新ノード`PhysicalContact34`、`.pgraph`保存・復旧、MIDI入力。
- `tools/contact34_demo.cpp`: 波形レンダリング、パッチ書き出し、CPU負荷測定。
- `tests/test_contact34.cpp`: 数値安全性・エネルギー・構造差・過去形式互換。

## 未検証項目

- Windows VST3 / pluginval / Studio One / JUCE GUI実画面
- レンダリングの聴覚評価、リアルギター音への同定、6弦間相互作用
- 非線形の指板衝突を高精度の接触方程式で解く方式
- 共鳴胴の3D FEMメッシュへの直接接続（現状3軸有効モデル）
- 多声・コード・連続奏法での実際の音楽的品質
