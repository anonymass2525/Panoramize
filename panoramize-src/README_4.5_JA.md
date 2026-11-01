# Panoramize 4.5.0 — Nonlinear Passive Boundary & Native-State Adapter

## 概要
**4.4までの疎行列空間FEMと、Native27の8つのエンジン種別のモデル代理状態（proxy）を、新しい同時陰的な非線形ソルバーに結合**する研究版。

重要：**既存Native27のVA・FM・声道フィルター・任意のスペクトルアルゴリズム自体を受動化したものではない**。任意DSPの厳密な受動性は一般に自明でも保証可能でもない。Native27の各 `Mode` を、正値質量・剛性・減衰・受動相反リンクを持つエネルギー正規化振動モードに**代替写像**し、元エンジンが必要とする能動駆動（VA/FMおよび`excitation>0`）は明確に別計上する。`strictPassive` は能動モード励振の指定を拒否する。発音自体へのMIDIノート励振は外部から入る能動仕事として記録する。

## 新しい空間非線形項
- **一方向接触:** FEMの任意の機械境界ポートに対し、`U=0.5*k*max(0,x-gap)^2` の一方向ペナルティ接触を定義する。**硬い接触の補完性制約・厳密な非貫通拘束は未実装**。
- **動摩擦:** 接触の正規反力 `N>=0` と接線の相対速度 `vt` から `F=mu*N*tanh(vt/v0)` の逆向き力を生成。`F*vt>=0` で散逸。静止摩擦・完全なstick-slipは未実装。
- **変形する3Dメッシュ:** 3D四面体FEMの節点変位を保持し、変形した頂点位置をm単位で読み出す。指定した四面体辺で**実際の変形後長さ**と基準長さを使った追加有限伸びエネルギー `U=0.5*k*(length-L0)^2` を計算。線形FEMの補助項であり、全面的な幾何学的非線形固体FEM、再メッシュ、自己衝突ではない。
- **受動非線形Native代理モデル:** エンジン種別はNative27のVA/FM/Spectral/Phase/Resonance/Physical/Vocal/Nonlinearの8種。周波数比・非負減衰・任意の非負quarticポテンシャルを保持し、Mode間の保存・散逸エッジを相反ポートとして組み立てる。**本来のNative27音響DSPの信号処理経路を実行しているわけではない**。

## 同時陰解法とエネルギー
線形FEMバックボーンは`SparseHamiltonian43::exportAssembled`から質量・CSR剛性/減衰行列を取得。8代理モードと受動エッジを同一システムに組み立て、非線形接触・摩擦・有限伸び・四次エネルギーを離散勾配で積分。固定点外反復と前処理付きCGを組み合わせる。非線形内反復が収束しないときは**状態を変更せず0サンプルを返し、失敗数を計上**する（すべての入力条件での安定性保証ではない）。

エネルギー収支 `E_final-E_initial-W_active+D_dissipated=residual` を監視し、入力された能動発振とMIDI励振は`W_active`、摩擦・減衰は`D_dissipated`に分離。毎サンプルの再確保は行わない。

## メッシュ境界
`.passive45`の`SPARSE43_BEGIN`〜`END`に1D・2D・3D空間形状と境界ポートを指定。`C`レコードが正規接触・接線摩擦ポート、`G`レコードが3Dメッシュ辺の有限伸びを指定。`NATIVE27_BEGIN`〜`END`は既存Native27グラフのモード・エッジを原形式で保持。入力の解釈は上の通りproxyである。

## Universal Patchbay
`Kind::NonlinearPassive45=23`、`.pgraph`ヘッダ`PANORAMIZE-GRAPH-45|1`、新発音ソース`NONLINEAR CONTACT + NATIVE PROXY / 4.5`を追加。既存4.4の音色・パッチは保持。**GUIのノード種類選択は追加したが、4.5接触/3D辺の専用GUI編集画面は未実装。Windows VST3ビルド/Studio One/pluginval未検証。**

## ビルドと再現
```
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build -j4
ctest --test-dir build --output-on-failure -E '^PANORAMA_DSP$'
./build/panoramize_passive45_demo --example test.passive45
./build/panoramize_passive45_demo test.passive45 test.wav 0.4
./build/panoramize_passive45_benchmark
```

## 評価上の制限
- 80DOFの4.5実験モデルはLinux 48kHz/64 samplesで約3.1ms/block、**実時間予算1.33msを超える**。高レイテンシのオフライン研究版。4.4高速縮約との同時組み合わせは未実装。
- 4.5の8Nativeモードはエネルギー正規化された**受動代理状態**であって、任意のNative27実DSPを受動的に接続した結果ではない。
- 動摩擦モデルは正則化摩擦、非貫通拘束はペナルティ近似。接触点は任意の離散ポートとして定義できるが、メッシュ形状に基づく接触点の自動検出は未実装。
- 3Dは既存小変形線形四面体FEM＋選択辺の非線形伸び補正。全体の大変形FEM、接触による局所再分割は未実装。
- 単音・短い人工音での検証。実録音、プロフェッショナル水準の発声・スペクトル品質、Windows Studio One実機、長時間・多発音・多数プラグインの実時間保証は未検証。
