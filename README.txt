
    mouse-processor  -- マウスボタンの割当変更、移動によるスクロール等

Introduction
 「mouse-processor」は、マウスボタンの割り当てを任意に変更したり、マウ
スの移動でウィンドウのスクロールを可能にするプログラムです。
2つのボタンの同時押しへの機能割り当てにも対応しています。
スクロール機能はトラックボールで利用すると効果的だと思われます。多分。

このソフトウェアは、無償、無保証、無制限にて公開します。
(つまり、パブリックドメインソフトウェア (PDS) と同等ということ。)


Requirements
 OS : Windows2000 以降


Install
 - mouse-processor の圧縮ファイルを任意の場所に展開させます。
 - mp.exe を実行させます。必要に応じてスタートアップ等に登録して下さい。


Uninstall
 - mp.exe を削除して下さい。不必要なら設定ファイルも削除して下さい。
   レジストリ等は使用していません。


Tasktray Menu
 タスクトレイのアイコンを右クリックするとメニューが開きます。各項目は
 以下の動作をします。

 - 再読み込み(R)
   設定ファイルを再度読み直します。設定ファイルについては Setting の項
   を参照して下さい。

 - 一時停止(P)
   動作を一時停止します。停止状態ではこのメニュー項目にチェックマーク
   が付き、再度選択すると動作が再開されます。
   動作を停止すると、マウス入力はプログラム起動前と同様の動作をします。

 - ログ(L)...
   ログウィンドウを開きます。ログについては Log の項を参照して下さい。

 - 終了(X)
   このプログラムを終了させます。


Setting
 - コマンドライン引数で絶対パスの設定ファイルを指定した場合は他の場所
   からの検索は行なわれません。
   コマンドライン引数で相対パスの設定ファイルを指定した場合はホームディ
   レクトリから検索されます。
   コマンドライン引数を指定しなかった場合はホームディレクトリから以下
   のファイル名を検索し、最初に見付かったファイルを使用します。

   - .mprc
   - dot.mprc
   - default.mprc

   ホームディレクトリとは以下のいずれかです。上から順に検索されます。

   - 環境変数 HOME のディレクトリ
   - 環境変数 HOMEDRIVE と HOMEPATH を繋げたディレクトリ
   - 環境変数 USERPROFILE のディレクトリ
   - プロセス mp.exe のカレントディレクトリ
   - 実行ファイル mp.exe と同じディレクトリ

 - タスクトレイのアイコンを右クリックして出るメニューから "再読み込み"
   を選択すると、設定ファイルを再度読み直します。

 - 設定ファイルの書式、内容については doc/CUSTOMIZE.html を参照して下
   さい。

 - 幾つかの設定ファイルを添付してあります。

   - default.mprc   一般的な設定。4ボタン以上のデバイスを想定。
   - 2button.mprc   2ボタンのデバイス用設定。
   - pressed.mprc   ボタン3を押下時のみスクロールモードとなる設定。
   - dot.mprc       ユーザごとの設定ファイルのテンプレート。

 - 設定ファイル default.mprc は、以下のような設定となっています。

   - 通常モード
     各ボタンのクリック   → そのまま
     ボタン1と2の同時押し → 縦横同時スクロールモードへの移行
     ボタン1と4の同時押し → 縦スクロールモードへの移行
     ボタン3と4の同時押し → ボタン5のクリック
     ホイール回転         → マウスポインタ直下のウィンドウへホイール
                            メッセージ送信
     マウス移動           → マウスカーソルの移動

   - 縦横同時スクロールモード・縦スクロールモード共通
     ボタン1クリック      → 通常モードへの移行
     ボタン1と2の同時押し → 通常モードへの移行
     ボタン3クリック      → スクロール速度を半減
     ボタン4クリック      → スクロール速度を倍加
     ボタン3と4の同時押し → スクロール速度を初期値へ戻す
     ボタン5クリック      → 通常モードへの移行
     ホイール回転         → マウスポインタ直下のウィンドウへホイール
                            メッセージ送信

   - 縦横同時スクロールモード
     ボタン2クリック      → 縦スクロールモードへの移行
     マウス移動           → マウスポインタ直下のウィンドウを縦横同時
                            スクロール

   - 縦スクロールモード
     ボタン2クリック      → 縦横同時スクロールモードへの移行
     マウス移動           →  マウスポインタ直下のウィンドウを縦スクロール

 - 設定ファイル 2button.mprc は、以下のような設定となっています。
   この設定を使うときは、default.mprc を先に読み込んで下さい。

   - 通常モード
     各ボタンのクリック   → そのまま
     ボタン1と2の同時押し → 縦横同時スクロールモードへの移行
     ホイール回転         → マウスポインタ直下のウィンドウへホイール
                            メッセージ送信
     マウス移動           → マウスカーソルの移動

   - 縦横同時スクロールモード
     ボタン1クリック      → 通常モードへの移行
     ボタン2クリック      → スクロール速度を半減
     ボタン1と2の同時押し → スクロール速度を倍加
     ホイール回転         → マウスポインタ直下のウィンドウへホイール
                            メッセージ送信
     マウス移動           → マウスポインタ直下のウィンドウを縦横同時
                            スクロール


Log
 タスクトレイのメニューからログウィンドウを開くことで、ログを参照する
 ことができます。ログには各種情報やメッセージ等が出力されます。

 - クリア(L)
   ログを消去します。
 
 - 詳細を表示(D)
   この項目をチェックすると、スクロールモード時等に情報が表示されます。

 - 閉じる(C)
   ログウィンドウを閉じます。


Known Problem
 - 他のマウスユーティリティーや、非標準のマウスドライバと同時に使用す
   ると動作が干渉し、予期せぬ挙動をする可能性があります。
   その場合は、他のマウスユーティリティーを終了、標準のマウスドライバ
   に変更するか、互いの動作に影響しないよう設定する等して下さい。

 - マウスコントロールパネルで左右ボタンを入れ替え (左きき用に設定) し
   ていても、mouse-processor を動作させるとその設定が有効になりません。
   (これは、ボタン入力のデータが
    デバイス → 入力キュー → mp.exe → 入力キュー → アプリケーション
    という経路をとるときに、入力キューに相当する部分でボタン左右入れ換
    え操作をされるため二重に入れ換え操作が入り、元に戻ってしまうため)
   左右ボタンを入れ替える場合は、mouse-processor の設定でボタンを入れ
   替えるか、mouse-processor を停止して下さい。

 - 他のプロセス、特に mp.exe よりも優先度が高いプロセスの負荷が高いと
   マウスポインタの動きが遅くなったり、動作が断続的になる場合がありま
   す。その場合は、mp.exe プロセスの優先度を高くすると改善することがあ
   ります。

 - window-scroll、neighborhood-scrollbar、scrollbar-control の各
   operator の drag、percentage、bar-unit モードではスクロールできない
   ウィンドウがあります。(MS Word、 65545行以上のエディットコントロー
   ル、UPX 等の packer を使っているもの等)
   その場合は、line-scroll または page-scroll モードを使用するか、
   wheel-message operator を使用して下さい。
   UPX 等で圧縮されている実行ファイルの場合は、展開すると改善される場
   合があります。(OpenJane 等は UPX で圧縮されて配布されているようです。)


TODO
 -
   設定に関するドキュメント
    -> 0.1.2 で一応追加。

 - キーボードとの連携


Link
 - TrackScroll <http://www.geocities.jp/makoko1974/ts.htm>
 - Wheel Ball <http://homepage1.nifty.com/jesus/>
 - X Wheel <http://www5b.biglobe.ne.jp/~hokko2nd/Windows/#XWheel>
   X Wheel NT <http://www5b.biglobe.ne.jp/~hokko2nd/Windows/#XWheelNT>
 - mouse-processor <http://www.tamanegi.org/prog/mouse-processor/>
