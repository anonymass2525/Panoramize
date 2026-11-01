# Panoramize v5.2 — Real-time-safe coefficient preparation, source transitions, external recording protocol and Unified GUI

## v5.1からの差分

- `RealtimeSafety52.h/.cpp`: 変更値の有限性・範囲検査、音声コールバック側の固定コスト指数平滑化。`publishBatch`は**全値を検査してから公開**するが、個別アトミック読み取り間で一瞬の中間状態を観測することがある。完全な複数フィールドのリアルタイム同時トランザクションではない。
- `SmoothGraphSwap52.h/.cpp`: 音声コールバック内で保持中ノート、ピッチベンド、CC1、サステイン等を新演算器へ渡し、12msの線形クロスフェードで構造切替。事前構築はUI/非音声スレッド、旧グラフの退役参照は原則次回UI公開時に回収。既存データを書き換える`tryUpdateEnergyGraph()`はUIから呼ばなくなった（同時実行のデータ競合リスクを避ける）。
- `PluginProcessor.cpp`: 構造差し替えを`graphSwap`経由に変更。旧オートメーションID・旧保存形式は残し、製品版識別子はv5.2独立 (`Pz52`)。
- `UnifiedParameterEditor50.h`: 共通インスペクターに検索とライブ制御可能な項目のみのフィルターを追加。DSPが許可しない形状・FEM等は従来どおり再準備。
- `joint_inverse50_cli.cpp`: `--infer-provenance` 追加。WAV/MIDI/楽器ID/録音セッションID/出典URL/ライセンスのTSVを受け、同じセッションが学習と未使用評価に分割されるデータ漏洩を拒否。出力は記録されたメタ情報、品質診断、6候補パッチ、学習・未使用評価誤差。
- `.github/workflows/windows-vst3.yml`: Windowsビルド、JUCE統合スモーク、VST3構造、pluginval strictness5検証を維持・5.2名称へ更新。**Windowsの実際の実行ログは未取得。**

## ビルド・テスト (Linux)

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build -j8
ctest --test-dir build -j4 --output-on-failure -R 'Safety52|LiveNative51|Recorded51|Unified50|AudioDesign49|GenuineNative47|Universal23|Sparse43|Reduced44|Energy40'
```

## 実録音の評価（実際の収録WAVは同梱していません）

`takes.tsv`の各行 (タブ区切り):

```
/path/recorded_note_A3.wav\t57\t1\tguitar-01\tsession-a\thttps://source.example/recording\tCC0-1.0
/path/recorded_note_C4.wav\t60\t1\tguitar-01\tsession-b\thttps://source.example/recording\tCC0-1.0
/path/recorded_note_D4.wav\t62\t0\tguitar-01\tsession-c\thttps://source.example/recording\tCC0-1.0
```

**上は入力例であり、存在するWAVや出典を示していません。** 各WAVはPCM16/24またはFloat32、22050〜96000Hz、モノラル／ステレオ、10秒以内、既知MIDI36〜96、同サンプルレートであること。`session-a/b/c`は独立の録音セッションID。

```
./build/panoramize_joint50 --infer-provenance takes.tsv results
```

**候補は音源構造の仮説であり、未知の形状・材質・声道の正解とは限りません。** 録音と出典の真偽はCSVからは検証できません。

現実の録音を使う際のデータ候補: University of Iowa MIS https://theremin.music.uiowa.edu/MIS.html または TinySOL https://zenodo.org/records/3685367 . こちらの環境では音源ファイルを取得・検証できていません。

## 機能範囲と未達

- Live coefficient test succeeds; **今回の新CoefficientBankは共通APIの研究実装**。全歴代DSP内部係数を直接このBankへ移す統合は未完了。Native27の5ms平滑化と一部ホスト用パラメーターは5.1から継続。
- グラフ切替は保有ノートを再発音させてPCMをクロスフェードするので、**物理エネルギー、各発振器位相、FFT窓、サステイン保持状態を完全に転送するわけではない**。連続音を意味論的に全く同じ楽器にしたわけではない。
- コールバックの`shared_ptr`原子操作がロックフリーである保証はない。UI側publish間隔と退役処理に制約があり、hard real-time証明ではない。
- 全エンジンの共通GUIとして、既存Patchbay+主な数値パラメーターのInspectorを提供する。全方式の専用3Dメッシュ/音声経路編集UIは未完了。
- 実録音による精度評価、Windows VST3、JUCE GUIのコンパイル、pluginval、Studio One実機試験は**このLinux環境では実行していない**。
