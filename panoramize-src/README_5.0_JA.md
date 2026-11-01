# Panoramize 5.0 — Identifiability-Aware Inverse Instrument & Unified Internal Controls

## どこまで実装したか

4.9までの機能を保持し、新規 `include/UnifiedControls50.h`, `src/UnifiedControls50.cpp`,
`include/JointInverse50.h`, `src/JointInverse50.cpp`, CLI、およびJUCE編集画面コードを追加。

### 共通の内部パラメーターID

* `controls50::catalog(graph)` は有効な発音ソースについて、ID/グループ/値域/単位/現在値/ライブ更新可能性を列挙。
* 対応: Globalマクロ、Patchbay node level、Native27（Direct/3.9/4.7経由）各Modeの周波数比・減衰・非線形・ピックアップ・能動励振、Genuine47トーン、2.9物理弦、3.2・3.3奏法、3.4接触、3.5多弦物理、4.0エネルギーポート、4.1音響境界、4.3疎行列FEM、4.4縮約、4.5非線形接触などの **主要な数値パラメーター**。
* `controls50::change` はIDから実際のGraphフィールドを編集する。`transaction` はコピー上で全編集を行ってから**実際の演奏エンジンにprepare可能か検証**して確定する。
* JUCE `UnifiedParameterEditor50.h` : 発音方式ごとに動的に変わる共通インスペクター。Universal Patchbayの `INTERNAL PARAMS 5.0` ボタンから使用。**JUCE/Windowsでのコンパイルと実機操作は未実施**。
* 既存の4.9のリアルタイム安全なホスト操作（マクロ、ノードレベル、Native47音色処理）は保持。**一般の内部パラメーターはグラフ再準備**であり、サンプル精度のホストオートメーションとは区別する。未登録の全ての内部処理まで網羅してはいない。

### マルチ録音からの実エンジン候補検索

`joint50::identify` は、2-12本のWAV（MIDI音高とtraining/heldoutが与えられている必要がある）を受ける。

1. 10個の正規化倍音記述子と5区間の音量包絡を比較し、録音の不明な総ゲインや位相に対して頑健な**限定的な特徴量**を利用。
2. Native VA/FM, 実Native27状態, Native vocal state, 2.9物理弦, Native+物理弦ハイブリッド, 4.4縮約FEMを既存の実発音器で再生して比較。
3. 限られた内部数値パラメーターを試行し学習誤差を下げる。未使用音高は**探索・ランキングに使わず**別指標として記録。
4. `*.pgraph` と共通コントロール `*.csv`、試聴 `*.wav` を候補ごとに出力。

この推定は演奏記録のMIDIノート情報に依存し、polyphonic transcription、声道形状、材質、3Dメッシュの真値は自動復元しない。旧4.8の有限の自由曲面FEM＋材質探索は引き続きソースに含まれるが、5.0のモード候補検索とはまだ一括最適化されない。学習誤差が小さい構造を、未知物体の正しい構造として宣言しない。

## 数学的な識別不能性

弦の基本周波数 `f0 = 1/(2L) * sqrt(T/μ)` だけを測っても張力 `T` と線密度 `μ` は一意に分離できない。両者を同じ倍率で変えても基音は変わらない。多くのFEM逆問題では、未知の励振、観測点、材料、境界条件の変化にも同様の曖昧さがある。発声でも単一マイク音声から声道・声帯・呼気条件を唯一の幾何学構造として特定できるわけではない。

## Linuxビルド・テスト

```
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --target panoramize_unified50_test panoramize_joint50 -j4
./build/panoramize_unified50_test
./build/panoramize_joint50 --demo examples50
```

実録音を用いるときは、`takes.txt` を `path/to/take.wav MIDI NOTE_TRAINING` 行で作成。最後の数値は `1` 学習、`0` 未使用評価。24bit / 16bit PCMまたは float32 WAV のmono/stereo、同一サンプルレートで、現在は1テイク0.2-10秒に制限。

```
./build/panoramize_joint50 --infer-manifest takes.txt output50
```

## 音源品質とホスト対応

本バージョンは音色アルゴリズムの全面改良ではなく、音色の比較・編集の接続性改善が主目的。旧音源の基礎音色品質・発声の自然性・未知構造復元の精度は改善保証なし。
Windows VST3、Studio One、JUCE GUI、pluginvalの新コード実機検証は未実施。Linux DSPテスト結果と製品動作確認を混同しない。
