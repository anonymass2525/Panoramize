# Panoramize 3.8.0 — Unified Multidimensional Physical Graph

## このバージョンで実装したこと

3.7の個別の1D/2Dレンダラーと3D FEM研究コアを足し合わせるのでなく、**各空間領域の変位・速度を一つの離散時間積分器で更新し、境界の力と反力を共通のパッシブ接続で交換する**新しい独立DSPカーネル `coupled38` を追加しました。

- `String1D`: 1D張力弦。格子点質量とばね。 
- `Tube1D`: 1D気柱の縦変位による質量／圧縮ばね。面積 A=0.001m² と空気密度 1.18kg/m³ を試作値に使い、境界の一般化力 F=A·pressure と速度 v をやり取りします。**正確な放射インピーダンスと圧力・流量ポートの全要素には未対応**。
- `Membrane2D`: 2D格子点の張力連成。
- `Plate2D`: 2D格子＋2格子先への追加曲げ近似ばね。**薄板の正確な双調和差分やシェルFEMではありません**。
- `TetraResonator3D`: 3D線形四面体FEMの体積・質量・弾性行列の組立。6成分の異方性倍率は Voigt 表記の剛性行列に正定値合同変換を適用。3.7のパラメトリック格子からメッシュを生成。**大変形・接触非線形は未対応**。
- `Connection`: 2つの離散境界を、保存ばねと散逸ダンパーで双方向結合。可変のレバー比 `ratio` は共役な力に同じ比率を掛けるため、リンクのポテンシャルエネルギーが非負になります。
- `Instrument::prepare`: グラフの型・範囲・メッシュ整合性・剛性と減衰の数値安定性を検査。力学方程式の行列・境界インデックスをオーディオ処理前に準備。
- `Instrument::processOne`: 各内部サブステップの全領域の力を集めて同時に更新。グラフの順序によるオーディオサンプル単位の遅延はありません。**陰解法や厳密な連立拘束方程式を解く方式ではなく、安定性を検証した陽的な時間積分です**。
- `Instrument::totalEnergy` / `domainEnergy` / `dissipatedEnergy`: 機械エネルギーと散逸の検査用。音声出力は現時点では変位を加算した研究用ピックアップです。
- `.phys38`: すべての領域・3D材質・空間位置・結合強度をテキストで編集・保存・読み込みできます。最大8領域、24リンク。格子とサブステップにも上限があります。

## ビルド・使い方

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build -j4
ctest --test-dir build --output-on-failure
./build/panoramize_coupled38 --demo demo38
./build/panoramize_coupled38 examples/physical38/all_five_reciprocal.phys38 my_sound.wav 1.5 48000
```

デモ4種:
- `string_membrane`: 1D弦⇄2D膜
- `string_plate_body`: 1D弦⇄2D板⇄3D四面体共鳴体
- `membrane_body_tube`: 2D膜⇄3D共鳴体⇄1D気柱
- `all_five_reciprocal`: 5種全領域と弦⇄管の追加循環リンク

`.phys38` の `node` は `family nx ny fundamentalHz damping exciter pickup amplitude xCells yCells zCells length width thickness camber taper young density poisson anisotropy[6]`、`link` は `from to positionFrom positionTo axisFrom axisTo stiffness damping ratio` を持ちます。単位は原則メートル、秒、ニュートン、kgであり、1D/2Dモデルには正規化された格子質量が含まれます。**物理量の定量校正が済んだ実物楽器モデルではありません**。

## テスト（今回のLinux実測）

- Release CTest: **30/30合格**（既存28 + 新DSP + .phys38 CLI）
- 新DSPテストの Clang ASan / UBSan: **合格**
- 保存した5領域レシピを読み込み直した音声WAVの SHA-256 が完全一致。
- 保存リンクの最大の相対エネルギー変動（初期約0.333秒の陽的積分）：**1.982%**。完全厳密保存ではありません。
- リンクを切った対照系と比較し、1D弦→2D膜、膜→板、板→3D、3D→管の各接続で領域間のエネルギー伝達を確認。
- 損失系は総エネルギーが低下し、散逸エネルギー累計が正となることを検証。
- 48kHz、1.5秒×4音声のLinux CLI全レンダリング約3.13秒（測定環境依存）。5領域1.5秒は約1.27秒。**全構成のリアルタイム演奏や大きな3Dメッシュの処理を保証しません**。

## 重要な未達・制限

1. 3.8の新コアは C++17 / CLI 研究版。既存JUCE VST3の `Universal Patchbay` と GUI から **`.phys38` を編集・演奏する機能は未統合**。Windows・pluginval・Studio Oneの実機検証は未実施。
2. 管の終端放射や開管、実物の流体‐構造連成、板の厳密な曲げシェル、膜の幾何非線形、3Dの摩擦接触・大変形は未完成。
3. 1D/2D格子は簡略化した質点・ばね近似、3Dは限定四面体FEM。これらの物性を現実の楽器として定量校正していません。
4. 数値的な接触点連成は同一サブステップで双方向ですが、厳密な陰的同時連立解法ではありません。高剛性・高減衰ではサブステップ上限に達した場合は安全のため構築を拒否します。
5. 形状逆推定器が 3.8新グラフのすべてのノードと結合を探索するわけではありません。従来の3.7探索器は保持しています。

3.7以前の過去版ソース・旧72音色・JUCEコードは変更せず同梱しています。
