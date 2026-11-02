# Panoramize v3.3.0 — Physical Articulation Engine

3.2の演奏可能な弦・ブリッジ・共鳴胴モデルに、**弦の内部状態へ作用する4種類の奏法**を新設しました。3.2のDSP、72 Factory Sounds、および旧 `.pgraph` を変更せず保持します。

## DSP上の演奏モデル

- **Hammer-on**: フレットでの接触時に弦振動モードへ正方向の速度インパルスを追加。前音の位相・エネルギーは保持し、弦の有効長を変えます。
- **Pull-off**: 押弦解除時の横方向弾きを負方向インパルスとして射影。これも弦をリセットせずに加算します。
- **Slide**: フレット位置の連続移動による有効弦長変化、運動速度に応じた接触摩擦励振。移動が止まれば摩擦励振も消えます。
- **Palm mute**: 弦長比0.70〜0.99（ブリッジ近傍が既定）の接触位置に対して、倍音ごとに異なる追加損失を適用します。単なるPCM音量フェーダーではありません。

いずれも**最大24の弦モードと簡易共鳴胴を使う物理的動機付けの近似**です。厳密な弦・指・指板・駒の3D接触力学（法線拘束、クーロン摩擦、弦の分布系、複数接触点の非線形衝突）ではありません。奏法の推定・逆合成は本バージョンでは更新していません。

## MIDI操作

Panoramize Universal Patchbayで `NEW PHYSICAL 3.3` を押すか、`Panoramize_Physical_Articulation_33.pgraph` をロードして演奏します。**Mono**モードの重複ノートでは、前の振動を引き継いだまま接触奏法を実行します。

- **CC20**: 次の重複 Note On の奏法。0–31 = 通常レガート、32–63 = ハンマリング、64–95 = プリング、96–127 = スライド。
- **CC21**: 次の接触／摩擦強度、0–127。CC20の**後に**送信します。
- **CC22**: パームミュート接触深さ、0–127。演奏途中に連続変更可能。
- **CC23**: パーム接触位置（弦長比0.70〜0.99）、0–127。
- 既存操作: Note Onベロシティ、Pitch Bend（±2半音）、Channel / Poly Aftertouch、Sustain CC64。

GUIの `EDIT ARTICULATION` は、新しい接触奏法の強度・位置・減衰・グライド・旧3.2パラメーターを編集できます。変更内容は `.pgraph` とホストのセッション状態へ保存する設計です。編集時は既存同様にグラフを再準備するため、演奏中のGUI編集に瞬断が起こる可能性があります。

## 3.3からの新形式

- `.art33`: 物理奏法モデル単体（`PANORAMIZE-ARTICULATED33-v1`）
- `.pgraph`: Universal Patchbay（`PANORAMIZE-GRAPH-33|1`）
- VST3識別子 `Pz33` / bundle `com.independent.panoramize.typegpt.v33`。旧版と同居可能な別プラグインにしています（旧版セッションへの自動差し替えではありません）。

## Linux DSPの構築

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build -j4
ctest --test-dir build -j4 --output-on-failure
mkdir -p audition
./build/panoramize_articulated33_demo audition
./build/panoramize_articulated33_benchmark
```

## Windows・Studio One

`.github/workflows/windows-vst3.yml` がVisual Studio 2022 + JUCE 8.0.16-securityでVST3、JUCE integration test、pluginval strictness5を検証する構成です。ただし**本セッションではWindows CI自体を実行していない**ため、成功を主張しません。手順は `docs/STUDIO_ONE_V33_ACCEPTANCE_JA.md` を参照してください。

## 未実装・限界

- 録音から「ハンマリング／プリング／スライド／ミュート」を自動認識し、それぞれの強度を推定する逆合成。
- 弦と指板の移動境界を厳密に解くFEM/FDTD、非線形弦摩擦の厳密な力学解法。
- 完全なギターの3D胴・指・ブリッジモデル。ブリッジ／胴は従来の3モード実効共鳴フィルタ。
- Windows VST3ビルドの成功、Studio One演奏確認、レイテンシ・CPU負荷の実機検証。
