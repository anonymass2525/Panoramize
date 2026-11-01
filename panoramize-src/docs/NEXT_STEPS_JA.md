# PANORAMA typeGPT 次の検証

優先順:
1. Windows x64でJUCE 9.0.4とVisual Studio/CMakeによるVST3 Releaseビルド。VST3ビルドエラーはこの時点で修正。
2. pluginval strictness 5、GUI込みテスト。
3. Studio Oneで全プリセットのロード、MIDI演奏、サステイン、ピッチベンド、ホストパラメーター保存復元、UIリサイズ。
4. ブラインド試聴による音色評価。Crystal VAは明瞭さ、Digitalは倍音美、Physicalは共鳴表現、Hybridは相互作用を重視。
5. 実時間CPU予算と安定性。特にフラクタル波形変形に対するオーバーサンプリングとAnti-aliasing、ハイサンプルレートでの安定性を改善。
6. パラメータースムージング、マルチモードフィルター、音源ごとのステレオ分離、物理共鳴モデルの高度化。
7. GUI Mod Matrixや実際にDSPグラフのノードを結線可能なGraph Engineを追加。

この試作はユーザーが実際に音を聴き、好きな音色や問題を報告してから改修する開発フローを前提とする。
