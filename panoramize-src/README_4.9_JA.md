# Panoramize 4.9.0 — Audio-to-Editable-Instrument & Common Host Controls

## 実装した一連のワークフロー

1. `panorama_audio_design49 --infer recording.wav out_dir` で録音から基本ピッチ、周期性、調波分布などを抽出。
2. 既存エンジンの**実DSPを短時間レンダリングして**、録音と基本音響特徴を比較。
3. 「Native27実VA/FM」「Native27直接音響状態」「物理弦」「Native＋物理弦」「1D/2D/3D縮約FEM」から最大5候補を返す。
4. `ranked_candidates.csv` に特徴距離・曖昧さを出力し、`candidate_N.pgraph` と `candidate_N_parameters.csv` を保存。
5. `.pgraph` をUniversal Patchbayに読み込んで同じ発音ソースのノード/モジュレーション/原音を編集・MIDI演奏する。
6. JUCEのUniversal Patchbayには `INFER WAV / PATCH` と `EDIT STRING` 操作を追加。前者はWAVの先頭最大2秒を解析して1位の候補を直接開く（GUIで複数候補を比較するUIは未実装）。後者で弦の基音・励振位置・ピックアップ位置・減衰・倍音数を編集する。

**これは任意の録音から真の物理構造を一意に推定するアルゴリズムではありません。** 一定条件の単音について既知の発音器を候補として比較する、限定的な前向き再合成による構造仮説選択です。スコアは独自の特徴距離で、統計的に校正された確率ではありません。

## 共通化した操作

- 既存全エンジンが使う Universal Patchbay の PCMレベルのグラフ/モジュレーションを維持。
- 新規 `universal23::Instrument::setHostParameterNormalized` は以下のIDを追加。
  - `0..8`: 既存Native47のTPTフィルタ/出力/4マクロ（維持）
  - `9..12`: 全発音器共通の4つのマクロ
  - `16..79`: C++ APIでの各ノード出力レベル（最大64ノード）
- JUCE AudioProcessorValueTreeStateには、追加の有効スイッチ `g49Enable` と、共通4マクロ・先頭8ノードの12ホストオートメーションパラメーターを公開。
- `g49Enable` は既定でOFF。旧セッションを壊さず、OFF時の原音は以前と同じ。
- audio `processOne()` 内では新しいメモリー確保なし。原子変数から共通マクロとノードの出力レベルを読む。
- `unifiedParameterRegistry()` がライブ変更可能な共通項目と、再構築が必要な方式固有パラメーターを区別してCSV出力。

### 重要な制限

既存全エンジンの内部材質・スペクトル・FEM係数・非線形状態が、すべて同一のDAWオートメーションIDに統合されたわけではありません。一般化の対象はまず共通マクロ/ノードレベル、Native47固有ホスト項目、編集可能なソースグラフです。弦の基音や材質変更は原則としてオーディオスレッド外でグラフを再準備します。GUIからのインポートはメッセージスレッド上で候補の再合成を行うため、操作中にUIが短く停止する可能性があります。

`AudioDesign49` の選択スコアは基音推定に依存するため、FMの変調が強いNative実音声ではピッチ推定が間違うことがあります。模擬Native録音では原ノートMIDI57をMIDI38と推定しました。候補ファミリーが上位であってもピッチ/物理パラメーターが正しいことを意味しません。

## CLI実行例

```
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build -j6
ctest --test-dir build --output-on-failure -E 'PANORAMA_DSP|Panoramize_Coupled38$|Panoramize_DeepInverse46$|Panoramize_MeshInverse36$'
./build/panorama_audio_design49 --demo demo49
./build/panorama_audio_design49 --infer my_recording.wav results49
```

WAV読み込みは22.05〜96kHz、PCM16/24またはfloat32、モノラル/ステレオ、0.25〜600秒（ファイル読み込みの容量上限あり）。GUI内は先頭2秒を使います。

## 検証環境 / 未検証

Linux GCC14 Releaseビルド・40件以上の既存CTestのうち39件を短時間回帰対象として**39/39合格**。長時間実行4件は未完了。Clang ASan/UBSanの4.9専用テストは合格。比較デモは人工弦およびNative47の自己生成WAVであり、実物楽器の実録音での頑健性・GUIのJUCEビルド・Windows VST3/pluginval/Studio One実機は未検証。録音から未知3D形状・材質・マイク位置を直接復元する機能は完成していない。既存4.8のFEM逆解析は別のオフラインCLIとして同梱。
