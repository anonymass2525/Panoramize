# Panoramize 5.1 — Live Native State Controls + Identifiability-Aware Recorded-WAV Validation

## 実装したこと

1. 既存Native27の実VA/FM発振器を保持し、**能動励振の倍率**をサンプルごとに平滑化して変更。0.5が1倍、0〜1が0〜2倍を表す。Vocal、Spectralなど非VA/FMモードでは能動励振のリアルタイム変更を拒否（既存Native27の元実装通り、それらは自動発振していない）。
2. Native27の**各モードのPCM読出し寄与倍率**を同じ0〜2倍のライブ制御に移動。Native26で音声取得を行う際に倍率を適用し、エネルギー保存境界にPCMを入力しない。
3. 共通パラメーターリスト`controls50::catalog()`へ16個の`native51.mode.N.pickupScale` / `native51.mode.N.excitationScale`を追加。`UniversalGraph::Instrument::setHostParameterNormalized()`の80〜95をNative27の実演算へ接続。`DirectState27`・`UnifiedPhysical39`・`GenuineNative47`を対応対象とする。複数ノードは同じモード番号へ一斉に適用。エンジン毎の独立レーンではない。
4. `Graph`へ新しい初期倍率2×8を追加し、非標準値があるときだけ`PANORAMIZE-GRAPH-51|1`と`NATIVE_LIVE`レコードに保存。従来値0.5のパッチは旧形式のままで、既存の読み込みを維持。サウンド生成が単純に同一なのか、新規モードによって変化したかをLinuxテストで測定。
5. JUCEホストパラメーターに`g51Enable`（初期無効）、`g51Pickup0..7`、`g51Excite0..7`を追加。AudioProcessorからライブパラメーターへ送る。値は5ms平滑化。JUCE/Windows実機ビルドは未実施。
6. `recorded51::inspect` / `auditCSV`: 外部WAVの音量、ピーク、DC、クリッピング、先頭20分の1区間RMS、注釈MIDIの周期性を監査。既存`panoramize_joint50 --infer-manifest`が`recording_quality_audit.csv`を出力。
7. `logSensitivityRank`: 任意の観測モデルの対数パラメーターに対し中心差分ヤコビアンを作り、局所ランクを判定。たとえば弦の基本周波数だけなら張力と線密度がランク1/2で区別不能になることを検証。**局所フルランクでも実物の3D形状・材質・声道構造の大域的に一意な復元を保証しない**。

## Linuxのビルド・テスト

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build -j6
ctest --test-dir build -j8 --timeout 15 --output-on-failure -E '^PANORAMA_DSP$'
./build/panoramize_live51_test
./build/panoramize_recorded51_test
./build/panoramize_joint50 --demo output-demo
```

- 45件の短時間回帰テスト: 45/45合格。全46件のうち既存の`PANORAMA_DSP` 1件は12秒制限でタイムアウト（失敗を切り分けるため除外）。
- 新録音診断モジュールとその専用テスト単体: ASan/UBSan合格。Native27と歴代全DSPを含むASan/UBSanフルビルドは実行時間制限により未完了。全DSPのサニタイザー合格を主張しない。
- DSPバイパス時、元Native27とサンプル単位一致。ホスト変更後は実音声が変化。`.pgraph`の保存・読み込みと制御値の再生を確認。
- Windows VST3、JUCE GUIコンパイル、pluginval、Studio Oneの検証は行っていない。

## 実録音による妥当性検証の方法

**このリリースで実録音そのものを用いた精度検証は行っていない。** テスト波形は再現可能な人工音であり、3テイクの演奏条件・MIDI音高・学習／未使用の区分は既知。

実際に録ったWAVや適切なライセンスの実演奏録音を使う場合は、各テイクを24,000Hz以上の1本のノートに切り出し、同じサンプルレートにして、下記のマニフェストを作る。

```
/path/take1.wav 57 1
/path/take2.wav 60 1
/path/take3.wav 63 0
```

```
./build/panoramize_joint50 --infer-manifest takes.txt output-real
```

出力: `recording_quality_audit.csv`, `ranked_structural_hypotheses.csv`, 6種の編集可能パッチと試聴WAV, `IDENTIFIABILITY_AND_LIMITS.txt`。

実録音の出典と許諾、演奏楽器、MIDI音高、マイク条件、学習と独立評価の分割を記録し、1回限りの一致を「完全復元」と解釈しないこと。手頃なCC0実録音資料例: https://github.com/facebookincubator/Meta-open-sound （実際のファイルを本配布物に同梱していない）。

## 未達項目

- Native27で数値演算器が準備する周波数比、減衰、非線形係数は再準備が必要。VA/FMの発振**駆動倍率**はライブ化したが、全モードの固有周波数／材質はライブ化していない。
- 3D FEMのメッシュや境界条件、声道形状等の構造編集は非リアルタイム。安全なグラフ交換はホスト実機で未検証。
- 実録音データに基づく精度指標は未取得。未知形状・材質・声道の一意かつ完全な同定は数学的にも保証できない場合がある。
- 主要DSPの網羅的なオートメーション、全発音方式の統一GUI、音質保証、DAW互換性保証は未完成。
