
1. Introduce Basic Bit Operations into String  --- 採択済み ([Feature #22118] / v1.md)
* bit_get, bit_set?, scalar bit_set/clear/flip, no-arg bit_count, bitwise_*。lsb_first: の可否とデフォルトはここで確定した。

2. Add Range Support to String Bit Operations
* bit_count(offset, length), bit_count(range), bit_set/clear/flip(offset, length), bit_set/clear/flip(range)
* 既存メソッドの arity 拡張だけなので、次に出しやすいです。ここで range parsing、負 range endpoint、範囲外の扱いを固める。
* bit offset の表現可能範囲は v1 と同じ 64-bit に統一済みなので、この点は v1 の説明をそのまま流用できる。

3. Introduce Bit Iteration Methods for String
* each_bit, bits, each_bit_offset, bit_offsets
* each_bit_offset は Arrow / BitTorrent / bitmap allocator の用途が説明しやすいので、each_bit と同じチケットでよいと思います。

4. Introduce Bit Run Methods for String
* bit_run_count, each_bit_run, bit_runs
* これは run-length という別概念なので独立させる。nil vs 0、byte boundary をまたぐ run、lsb_first: false の説明が必要になるので、iterator 本体とは分けたほうがレビューしやすいです。

5. Introduce Bit Slicing for String
* bit_slice(offset, length), bit_slice(range)
* これは「結果 String の packing をどうするか」が論点になるので単独がよいです。特に「指定座標は lsb_first: に従うが、結果は物理 bit sequence を保持する」という説明が必要。

6. Introduce Bit Splicing for String
* bit_splice。
* bit_slice の逆操作として出す。bytesplice と違って resize しない理由、source/destination の範囲外エラー、round-trip propertyを中心に説明する。

7. Introduce Array#mask and Array#mask! with String Bitmaps
* 親チケット外の新提案として、String bit API がある程度固まったあとに別 Feature として出すのがよいです。String に閉じた話ではなく Array への拡張なので、親 #22082 の子というより sibling ticket が自然です。

Array#mask は、少なくとも lsb_first: の名前とデフォルトが決まってからがよいです。bit_slice/splice までは待たなくてもよく、v1 とrange/iterator あたりが通れば十分に説明できます。逆に Array#mask を早く出しすぎると、String bit API 本体の議論が Array の API設計に拡散しそうです。

タイトル案:

Introduce Basic Bit Operations into String
Add Range Support to String Bit Operations
Introduce Bit Iteration Methods for String
Introduce Bit Run Methods for String
Introduce Bit Slicing for String
Introduce Bit Splicing for String
Introduce Array Masking with String Bitmaps

ポイントは、bit_slice と bit_splice を最後にすることです。ここは一番仕様密度が高く、先に scalar/range/iteration が合意されてい るほうが説明しやすいです。
