# wanictf-2024_promise

---
title: promise
level: 5
flag: "FLAG{pr0M1S3s_@ND_a5YnC'n_@w@17}"
writer: southball
---

# promise

## 問題文 / Statement

JavaScript の Promise について勉強した。なんかいろいろできますね！

I just learnt about JavaScript promises. They are a very powerful construct!

## 解法 / Writeup

I know that the problem is solvable, but it is quite annoying. Many people used Z3 to solve the transformed program, which is perfectly fine if it works.  
この問題が解けることがわかっているが、かなりめんどくさい。Z3 で解いた人が結構いるらしいで、解けたら OK！って思う。

I will explain the overall structure of the program, and share some tricks that I think can be used to solve this problem.  
プログラム全体の構造を説明して、解くとき使えそうなトリックを説明する。

The core idea of this challenge, is that you can use promises to model a write-once, read-many memory, and you can wait for the memory to be written. This is demonstrated in the program below.  
この問題を作った発想は、Promise を使うと一回書き込めるけど何回も読み込めるメモリーを作れる。しかも書くまで待つこともできる。以下のプログラムを読むとわかるかも。

```js
let writer;
let reader = new Promise((resolve, _reject) => {
    writer = resolve;
});

(async () => { console.log(await reader); })();

writer("Hello, world");
```

If you execute the program above, you will see `Hello, world` outputted. You can also `await` multiple times, and get the same value always.  
このプログラムを実行すると、`Hello, world` が出力される。何回 `await` しても同じ値が出てくる。

So the challenge program consists of two parts:  
問題のプログラムは二つの部分から構成されている：

1. Create all the readers and writers.  
   Reader と Writer を全部作成する。
1. Use the readers and writers to do some calculation.  
   Reader と Writer を使って、何らかの計算をする。

We could use an array but it would be too easy 😅 so every entry of the array is extracted into a single variable, and the variable names are obfuscated. Also, since the calculation does not need to be done in order, they are shuffled.  
配列を使ってもいいけどそうすると簡単すぎる 😅 ので、配列の各要素を変数にして、変数名を難読化した。そして、計算は任意の順番でできるので、シャッフルした。

To solve this problem, we basically want to transform the program from async-ful to async-less. (There are no write-loops, i.e. writing to the same location multiple times in this program.) There are only a few types of calculations in this program, so you can replace all of them, and do a topological sort, and obtain a async-less program. Some participants used regular expression which is alright, but you can also use [ast-grep](https://ast-grep.github.io/) which should be more flexible in terms of syntax.  
この問題を解くために、promise がない等価なプログラムにしたい。（このプログラムは write-loop、つまり同じところに何回か書くことがない。）そして、このプログラムは何種類かの計算しかないので、全部書き換えて topological sort すると、async がないプログラムにできる。正規表現を使った参加者もいるが、[ast-grep](https://ast-grep.github.io/) を使うとより柔軟な変換ができるはず。

The encoded program actually consist of 3 parts:  
エンコードされた計算は、三つの部分から構成されている。

1. Read all 32 characters of the flag, and push `i * 173 + c[i]` to an array.  
   フラグの 32 文字を読み取り、`i * 173 + c[i]` を配列に挿入する。
1. (Bubble) sort the array using only `min` and `max`.  
   `min` と `max` のみで配列を（バブル）ソートする。
1. Extract each bit of the sorted array, and compare it against a known array.  
   配列の各ビットを抽出し、既知の配列と比較する。

Using `c[i] + i * 173` might break `angr`-based solution. Not sure...  
ここで `c[i] + i * 173` にしてたら `angr` で解けなかったかも。試してみないとわからない...

The flag is `FLAG{pr0M1S3s_@ND_a5YnC'n_@w@17}`.  
フラグは `FLAG{pr0M1S3s_@ND_a5YnC'n_@w@17}` である。

Trivia: this challenge is already called `gates_js`, while `gates` is called `gates_c`.  
どうでもいいこと：この問題は元々 `gates_js` という名前で、`gates` は元々 `gates_c` だった。
