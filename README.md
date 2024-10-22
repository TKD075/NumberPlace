# NumberPlace
ナンプレ(数独)を自主制作しつつGitHubを使ってみる

## ナンプレとは

- ナンプレ(Number Place, Sudoku)とは、3* 3のブロックに区切られた9* 9のマスに1~9までの数字を埋めていくパズルのこと。
- 「数独」は日本のパズル制作会社ニコリが商標登録をしている
- 予め埋められているマスの数字および自らで埋めた全ての数字に対して、後述する条件に矛盾しないように81マス全てに数字を埋めることができるとクリアとなる。

## 条件
- どの縦列のブロックに対しても、それぞれ1~ 9の数字が1個ずつ入ること
- どの横列のブロックに対しても、それぞれ1~ 9の数字が1個ずつ入ること
- どの3*3のブロックに対しても、それぞれ1~ 9の数字が1個ずつ入ること
  - この3* 3のブロックは、9* 9のブロックを3* 3の正方形ブロックで9等分したブロックのことを言う

## 目標

1. GitHubを使うにあたって必要な操作や機能の習得
2. ナンプレの問題を自動生成するプログラムの作成
3. ナンプレのWebアプリケーション化
4. Android及びiOSでアプリケーション化

## 実装済みの機能

 - 盤面の自動生成機能
    - 規定の盤面を1つ作成してあり、この盤面を3列または3行ごとにシャッフルすることで理論上6^8通りの盤面を作成可能
 - 数字キー、テンキー、数字ボタンでの数字入力
 - メモ機能
    - メモボタン押下で数字ボタンが灰色に、マス内にメモを保存可能
    - 条件を満たさない数字はメモできない
 - 消去機能
    - eraceボタン押下で現在選択中のマスの誤った数字またはメモに残している数字を消去
 - オートメモ機能
    - automemoボタン(見切れている)押下で全てのマスにメモを入力
 - ヒント機能
    - fillボタン押下で現在選択中のマスに解答を入力
 - ポーズ機能
    - ポーズボタン(タイマーの右側)押下で一時停止
    - 画面を隠して一時停止中に解くことができないようになっている
 - タイマー機能
    - 盤面が表示されてからの経過時間を計測
    - ポーズ中はタイマーが止まる

## 未実装の機能

 - レーザー機能(選択中のマスのブロックと行列と数字をハイライト表示)
 - 1つ戻る機能
 - ミスカウンタ機能
 - メモを含め、入力不可能な数字を入力しようとしたときに警告音を鳴らす機能
 - ウィンドウを閉じる前に確認のワンクッションを挟む機能
 - 特定の数字を9つ埋めた場合に、その数字のボタンを消去する機能
 - 難易度管理機能
 - 盤面作成段階で、残した数字でただ一通りの解答を導けるか確認する機能
 - fillボタンではなくヒントを出す機能
   
