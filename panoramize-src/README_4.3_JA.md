# Panoramize v4.3.0 — Adaptive Sparse Acoustic–Physical Graph

## 4.2からの変更
4.2の任意1D線分・2D三角形・3D四面体・0D空洞の疎ポート・ハミルトン系を保持し、別発音ソースとして `sparse43` を追加。音質・物理的整合性優先の研究実装。**実験で速かった構成と、まだリアルタイムで動かない構成を明確に区別する**。

- `SparseHamiltonian43`: 準備時に不完全コレスキー IC(0) を構築、SPD因子化に失敗した場合はJacobi前処理へ戻す。準備済みのCSR疎行列と三角因子を使用し、`processOne()` 内に動的メモリー確保はない。
- 前処理選択: `0=Jacobi`, `1=IC(0)`, `2=自動`（128 DOF未満はJacobi、それ以上はIC(0)を試行）。この閾値は測定結果を元にした暫定的なヒューリスティックで、機種依存の最適値ではない。
- `AcousticTube1D`: 音響管を1D線分要素で表現。質量 `rho*dx/A`、圧縮剛性 `rho*c²/(A*dx)`、状態変数は体積変位 `q [m³]` と体積速度 `q' [m³/s]`。単純な線形の無損失波動と指定減衰に対応し、開管放射・粘性境界層・音響流れは未実装。
- `ModalResonator`: 複数の線形共鳴モードを独立した振動自由度として組み込む。
- `SpectralModal`: 有限個のスペクトル部分音を位相を持つ線形モードとして組み込む。**FFTスペクトルや既存Native27の任意のスペクトル処理を直接パッシブ化したのではない**。
- `PortUnit::NormalizedModal`: エネルギー正規化座標 `q [sqrt(J)·s]` と速度 `v [sqrt(J)]`。リンク側で `modalScaleA/B`（相当変位に換算する係数）を明示し、機械変位/体積変位へ正・逆同一変換を適用。係数は実験的に校正していないため、物理現象の定量的再現ではない。
- `SparseHamiltonian43` を Universal Patchbay の新ソース Kind=21として追加。`PANORAMIZE-GRAPH-43|1` の `.pgraph` に `SPARSE43_BEGIN/END` を埋め込む。詳細な疎FEMメッシュをGUIから直接生成・編集するパネルは未実装。

## パッチの仕様
- 単独の `.sparse43` レシピは、モードごとの `M frequencyHz damping`、領域のモード数、リンクごとの`modalScaleA`, `modalScaleB`、`preconditioner`値を含む。
- `.pgraph` では新しい Kind=21 のソースをMIDIで発音。
- 旧4.2 `.sparse42` とKind=20を**改変せず残している**ため旧パッチは従来エンジンで読み込める。旧フォーマットは新型の管やモードを表現できない。

## 演算式
`H=.5 pᵀM⁻¹p+.5 qᵀKq`、`q'=M⁻¹p`、`p'=-Kq-RM⁻¹p+B u`。
結合について保存部分 `K += k CᵀC` と散逸部分 `R += d CᵀC` を共通のグローバル行列へ組み立てる。
端面の力と速度は転置写像を通して交換する。対称正定値の陰的中点線形系を共役勾配法で解く。IC(0)は前処理であり、直接法ではない。

## ビルドとCLI
```
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build -j4
ctest --test-dir build -E '^PANORAMA_DSP$' --output-on-failure
./build/panoramize_sparse43_demo --example example.sparse43
./build/panoramize_sparse43_demo example.sparse43 out.wav 1.4 48000
./build/panoramize_sparse43_benchmark
```

## 検証範囲
Linux Releaseの旧34件＋新1件（時間がかかる旧`PANORAMA_DSP`を除く）：35/35合格。
Clang ASan/UBSanの4.3専用テスト：合格。全36件中の旧`PANORAMA_DSP`テストはこの実行では**完了未確認**。Windows VST3、JUCE GUI、pluginval、Studio One実機は**未検証**。

4.3の低次元モデルは72 DOF/476 NNZ、拡張モデル451 DOF/1901 NNZ。48kHz/64sampleの単体ベンチでは451 DOF・IC(0)は約1,353µs/ブロックで、予算1,333µsより少し長い。大規模マルチボイスの安定したリアルタイム再生は未達。音響放射、厳密な非線形接触、時変メッシュ、ポリフォニックな物理構造の複製、すべての既存音響状態の受動接続も未完成。
