# Panoramize v4.0.0 — Energy-Based Unified Ports

## 目的
3.9の双方向「能動トランスデューサー」の研究を踏まえて、SI単位の相反ポートに基づく、
受動保存接続 / 散逸接続 / 能動励振を区別した**新しい低次元の同時陰的連成ソルバー**を追加。
3.8の分布空間格子FEM全自由度や2.7のNative Stateモードをこの陰的ソルバーに丸ごと組み込んだわけではありません。

## 単位と結合
* 機械ノード: 変位 `q [m]`, 速度 `v [m/s]`, 力 `F [N]`, パワー `F v [W]`。
* 音響ノード: 体積変位 `Q [m³]`, 体積流量 `U [m³/s]`, 圧力 `p [Pa]`, パワー `p U [W]`。
* 接続に指定した有効面積 `A [m²]` で、音響体積変位を機械変位へ変換: `x = Q/A`; 反作用は面積変換により仕事を保存。
* 接続の一般化変位差を `e=c_i q_i-c_j q_j` として、保存ばね `E=k e²/2` と散逸ダンパー `D=d(de/dt)²` を区別。
* 音響ノードの質量欄は機械質量[kg]ではなく音響慣性 `Pa·s²/m³` に対応。現在は低次元の等価振動モデル。
* ノード自身のダンピングは物理・音響ノードに適切な一般化単位で指定。

## 同時陰的連成
陰的中点法: `q(n+1) = q(n) + h v_mid`, `v(n+1)=2v_mid-v(n)`。
`(2M/h + D + hK/2) v_mid = (2M/h)v(n)-Kq(n)+f_active`。
Kは保存ポテンシャル、Dは散逸接続、f_activeは外部駆動。
最大12領域、24接続で全K・D行列を組み立て、対称正定値Choleskyを毎サンプル解く。
正の質量・非負剛性・非負ダンピングを事前チェック。ノード位相を持ち越す。
音声へのreadout `tanh` は力学系へのフィードバックを持たない独立した観測器。

エネルギー簿記は `E_final - E_initial - W_active - W_parameter + E_dissipated = residual`。
ここで `W_parameter` はサンプルごとの連続的パラメータ変更が系へ加えた/奪った仕事。
これはソルバー内部の全一般化状態に関する検証で、現実のギター・室内音場の物理精度を保証するものではありません。

## リアルタイム変更
* 数値のみ（各領域の周波数・減衰、リンクの剛性・減衰、GUI座標）を変更した場合、既存演奏状態を保持。
* 接続係数の変更は `std::atomic<double>` により音声スレッドから参照し、20ms（変更可能）の指数平滑化。
* 構造、領域の単位種別、質量、面積、音声出力係数、励振方式を変更した場合は準備済みグラフを丸ごと安全に差し替える。現在はその際に発音状態がリセットされる。
* 波形計算 `processOne()` 内では動的メモリー確保を行わない。
* パッチのホスト自動化パラメータとしてはまだ公開しておらず、編集画面からの操作とテキストレシピに限定。

## GUI
`NEW ENERGY 4.0` → `EDIT ENERGY GRAPH`。
キャンバス上でノードをドラッグ、CONNECTを押して接続元→先をクリック、接続線をクリックして削除・編集。
ノードをダブルクリックすると励振ノード。音響/機械のタイプ切り替え、周波数・質量・減衰・有効面積・出力係数・能動入力を編集。
物理グラフは`.energy40`、Panoramize全体のグラフは`.pgraph`として保存。
JUCE GUIについてはコード追加のみで、Linux JUCE/Windows実機コンパイルは今回実施していません。

## 既存プラグインと互換性
既存の3.9および過去の全発音器を削除せず、新しく `Kind::EnergyGraph40=18` を追加。
既存 `.pgraph` のヘッダを引き続き受理し、新形式は `PANORAMIZE-GRAPH-40|1` を使用。
Windows VST3識別子は `Pz40`。まだWindowsビルド / pluginval / Studio One実機検証をしていません。

## 検証
* Linux Release: CTest 33/33 合格。
* Clang ASan/UBSan (新機能専用): 合格。
* エネルギー残差: 保守的テストで約 `10^-19` J、平滑化を含めたテストでも約 `10^-20` J。
* WAVと`.pgraph`は同じレシピでバイト単位一致。
* 48kHz/64サンプル: 12領域/17接続で中央値約84us; 予算1333us。ただし仮想CPU上の単体DSP測定。

## ビルド
```
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build -j4
ctest --test-dir build --output-on-failure
build/panoramize_energy40_demo --write-example demo.energy40
build/panoramize_energy40_demo demo.energy40 demo.wav 1.5 48000
build/panoramize_energy40_benchmark
```
WindowsでのJUCE VST3は `-DPANORAMA_BUILD_VST3=ON -DPANORAMA_JUCE_PATH=<JUCEソースの場所>` により要ビルド・検証。
