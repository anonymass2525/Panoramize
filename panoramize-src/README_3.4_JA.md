# Panoramize v3.4.0 — Contact Dynamics / Spatial String

この版はv3.3の24倍音モーダル近似とは別の新しい物理発音系 `contact34` を追加する。以前の72ファクトリー音色やGraph 3.3形式は削除しない。

## 発音の数理モデル

- 1本の弦の**横振動変位／速度**を40個の内点で空間差分化。隣接点の張力による反力を直接計算。
- ナットは固定端。ブリッジ端は独立した質量点。ブリッジから3つの有効胴振動方向への弾性結合は、作用反作用の符号が整合する同一の連成系で計算。
- 指の押弦位置はMIDIノートとピッチベンドで決まる弦長方向の座標として連続移動する。位置周辺のばね・ダンパー拘束で指による接触を近似。
- 指板は**片側障害物**として弦の負方向変位にのみ接触ばねが働く。指板摩擦・パームミュートは位置に依存する速度散逸、スライド摩擦は手からの能動入力として扱う。
- ハンマリング／プリングは指位置で弦の速度状態へ局所インパルスを付与する。レガート移動でも弦の既存変位・速度をリセットしない。
- `velocity-Verlet` と減衰分割による数値積分。準備時に時間刻みと安全性を検査。実行時メモリー確保・mutex・ファイルI/Oなし。
- 固定指が動かない／減衰なし等の制限条件で機械エネルギーの安定性を試験。**指の移動、外部励振、摩擦を含む全系で厳密なエネルギー保存を保証するものではない**。

## 主な限界

1. **1本の横振動弦の空間格子＋3自由度の有効共鳴胴**。6弦の現実のギター全体、弦の長手方向・ねじり方向、3Dの柔軟な胴体メッシュを直接計算する段階には未到達。
2. 指の接触は有限剛性の近似であり、クーロン摩擦や衝突接触相補性問題を解く厳密な接触ソルバーではない。
3. 弦の剛性に由来する高次の非調和性、ボディの放射特性、ピックアップ電磁特性、弦ごとのノイズまで完全な物理同定はしていない。
4. Universal PatchbayのノードとしてMIDIを受信し `.pgraph` ファイルとDAWプロジェクトへ保存するコードは実装したが、**WindowsのVST3ビルド・pluginval・Studio Oneの実機テストは未実施**。
5. 単体ベンチマークで8音は目標バッファ時間以内、16音では超過したケースがある。ライブ演奏用途に対する最悪実行時間保証はまだない。

## Studio One 導入前の手順

既存のPANORAMIZEのVST3と別IDで共存するため、VST3メタデータは `Pz34` / `com.independent.panoramize.typegpt.v34` とした。

1. Windows GitHub ActionsまたはWindows CMake + JUCE環境でVST3をビルドする。
2. Windows側pluginvalを実行し、失敗時はリリースせずログを確認。
3. Studio Oneで新VST3をロードし、`UNIFIED ACOUSTIC PATCHBAY` → `NEW PHYSICAL 3.4`、または `examples/Panoramize_Spatial_String_34.pgraph` をロードする。
4. 単音で音声・保存復元・ノートオフを確認。Pitch Bend、CC20（奏法）、CC21（強さ）、CC22（パーム深さ）、CC23（位置）の順に試す。
5. 演奏途中のGUI変更は発音器の再準備が発生する場合があるので、安定化が済むまで録音中の連続操作を控える。

## ソースと再現

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build -j4
ctest --test-dir build --output-on-failure
./build/panoramize_contact34_demo output_dir
```

`output_dir` はあらかじめ作成する。`contact34_demo` は24bit・48kHzの6つのWAV、パッチ、設定、ベンチマークCSVを出力する。設定ファイルは `PANORAMIZE-CONTACT34-v1` でパラメーターのバージョンを管理する。
