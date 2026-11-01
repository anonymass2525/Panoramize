# Panoramize Prototype 2.8 — Temporal Inverse Synthesis

## 何が新しいか

2.0 までの `GraphPatch` の 4,096 フレーム単音探索とは別に、**v2.7 の編集可能な `native27::Graph` 自体を逆設計**する C++17 のオフライン探索器を追加。

- 入力は最大8個の学習録音と8個の独立検証録音。**録音ごとの長さは 1,024フレーム以上、12秒以下、8〜48kHz**。非破壊的なオフライン処理。
- 25ms単位の連続時系列を維持して特徴を抽出（時間平均で潰さない）。RMS、波形差分、高速なゼロ交差率、クレスト、4つのGoertzel定点帯域で**アタック、サステイン、リリースの時間差**を評価。
- `NoteOn / NoteOff / PitchBend / ModWheel` のイベント列を入力としてリレンダリングする。重なるNoteOn/Off（ポリフォニックな重複発音）、ピッチベンドで作るビブラートを評価可能。
- 2.7 の VA・FM 能動励振と Physical/Spectral/Phase/Resonance/Vocal/Nonlinear モードの種類・パラメーター・保存/散逸結合・接続を予算付きで変異探索。
- 探索後に初めてholdoutを処理（**ホールドアウトを候補選択に使わない**）。結果を CSV と 2.7 JUCE Universal Patchbay でロードできる `PANORAMIZE-GRAPH-27|1` `.pgraph` と各音源のWAVで保存。
- 初期候補8本とトポロジー／連続パラメーターの確率変異による、固定予算内の探索。完全探索ではない。トポロジーハッシュで代替グラフを区別。

## 明確な限界

- **MIDIイベント／ビブラート／ノート遷移は音声から自動推定しない。** 入力に付随するイベント注釈を必要とする、条件付き逆合成。2.8での「奏法解析」はそれぞれの奏法での音色変化を評価すること。
- 実際のレガート（スラー、モノフォニック・ポルタメント等）の物理モデルは未搭載。検証は重なったノートの発音。
- 25ms区間の4固定周波数のスペクトル記述子は**限定された知覚特徴量**。時間領域波形、全スペクトル、包絡の知覚的マッチ、実楽器の同一性を証明しない。
- 同期した録音で、ドライモノラルを想定。録音開始位置ずれ、残響、雑音、強いマイクロフォン変化には弱い。
- 探索は構造を生成するが、最良解・実在楽器の物理同定・史上初の楽器発見を保証しない。形状／材質／励振位置の最適化は未実装。
- 多くの候補について独立にDSPをコンパイルするので**オフライン専用**。MIDIイベント処理とプラグイン演奏系は旧実装と同じ。
- Windows MSVC / pluginval / Studio One に関する実行試験は**未実施**。VST3のバンドル ID / plugin code を `com.independent.panoramize.typegpt.v28` / `Pz28` に変更し、旧VST3との共存を意図。

## デモ実験（合成ターゲット、実録音ではない）

- 12kHz、各**2.2秒**、学習3回（短いゲート＋減衰／5Hzのピッチベンドビブラート／ノート重複）、未知1回（別音高・強弱・ゲート長）。
- 元の物体は `FM + Vocal` の2モードと保存結合からなる 2.7 新形式 `native27::Graph`。検索の初期テンプレートとは比率・減衰・外部励振・接続係数が異なる。
- 48候補を実際に描画し、差分を評価。提案2種、構造署名が異なる。以下の数値は数値特徴量損失であり知覚品質ではない。
- 1位: 学習 1.96812、未使用 1.26825。
- 2位: 学習 2.09100、未使用 1.13259。検証値が一位より低くても、holdoutを使って順位の入れ替えはしない。
- 実験の48kHz 16bit WAVは**解析・探索とは別に生成**。同じ演奏イベントで、原音／発見パッチを収録している。聴取試験はユーザー自身の評価が必要。
- 処理テスト: Linux Release CMake＋GCC、**CTest 17/17**。`panoramize_temporal28_test` は Clang ASan/UBSan でも合格。
- 既存の旧ソースをすべて書き換えたのではなく、2.7のDSPとGUIを保持したまま、独立した新しいオフライン探索器を追加。

## 使い方

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build -j4
ctest --test-dir build --output-on-failure
build/panoramize_temporal28 --demo demo 48
build/panoramize_temporal28 demo/performances.txt found_instruments 48
```

テキストマニフェスト（WAVへの相対パス。単位は **WAVのサンプルフレーム**）：

```
TAKE|train|clip1.wav
EVENT|0|ON|60|0.80
EVENT|24000|OFF|60|0
END
TAKE|train|clip2.wav
EVENT|0|ON|55|0.60
EVENT|6000|BEND|0|0.5
EVENT|20000|OFF|55|0
END
TAKE|holdout|unseen.wav
EVENT|0|ON|67|0.75
EVENT|15000|OFF|67|0
END
```

- `BEND` は半音単位、`WHEEL` は0～1。イベントはフレーム昇順。
- WAV はPCM 16bit / 24bitまたはfloat32、mono/stereo（ステレオ時は平均化）。8kHz〜48kHz。
- Studio Oneへは v2.7 / v2.8 のUniversal Patchbay画面の **LOAD GRAPH** から推定 `.pgraph` を読み込み、MIDIで演奏。2.8 VST3はWindows未検証。
- 学習中の録音やモデル改変はオフラインコマンドラインで実行し、VST3オーディオコールバックで探索はしない。

## 次の研究

現状は **event-conditioned temporal inverse design**。次の大きな課題は、録音だけからノート境界・連続的なF0・表情・励振推移を推定し、推定不確実性を伴う演奏推定と物理／スペクトルグラフ探索を共同最適化すること。実楽器の録音に対応するには、混響・ノイズモデル、マルチレゾリューションSTFT、聴感重みづけ、複数マイク配置も必要。
