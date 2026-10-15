# suctf 2026 SU easygal writeup

# Esaygal Writeup

## 题目机制

当前版本已经不是 `14` 节点小规模枚举题，而是一个 `60` 节点、每步二选一的线性优化题。

核心判定在 [`GameManager.cs`](/e:/Unity%20down/esaygal/Assets/Scripts/Runtime/GameManager.cs)：

- 全部 `60` 个节点都要走完
- 最后统一结算
- `currentWeight > maxWeight` -> 失败结局
- `currentWeight <= maxWeight && currentValue == trueEndingValue` -> 真结局

其中当前数据的参数在 [`story.json`](/e:/Unity%20down/esaygal/Assets/Resources/Story/story.json) 里：

```json
"meta": {
  "maxWeight": 132,
  "trueEndingValue": 322,
  "nodeCount": 60,
  "verificationMethod": "DP count exact optimum paths"
}
```

重点是：真结局唯一性不靠 `flags.Contains(...)`，只靠 `weight/value` 保证。

## 为什么不该再用穷举

如果直接枚举所有路线：

- 每步 `2` 选 `1`
- 共 `60` 步
- 总路径数是 `2^60`

也就是：

```text
1,152,921,504,606,846,976
```

这个规模已经不适合朴素爆搜。

自然解法应该是 DP。

## 正确建模

把题目视为：

- 有 `60` 个阶段
- 每个阶段必须二选一
- 总权重不能超过 `132`
- 目标是让总价值最大

定义：

```text
dp[i][w] = 前 i 个节点、总权重为 w 时能取得的最大 value
cnt[i][w] = 达到 dp[i][w] 的路径条数
```

转移很直接：

- 对第 `i` 个节点的两个选项分别尝试
- 更新新的权重 `w + choice.weight`
- 取更大的 value
- 如果出现相同最优值，则累加路径数

最后在所有 `w <= 132` 中取最大 value，并统计达到这个 value 的总路径数。

## 仓库内验证器

仓库已经自带独立验证脚本：

- [`Tools/Verify-StoryUnique.ps1`](/e:/Unity%20down/esaygal/Tools/Verify-StoryUnique.ps1)

直接运行：

```powershell
./Tools/Verify-StoryUnique.ps1
```

实际输出：

```text
Verification complete.
NodeCount=60
MaxWeight=132
ConfiguredTrueEndingValue=322
BestValue=322
OptimalPathCount=1
UniqueSolution=True
OptimalWeight=132
UniquePath=N1B,N2B,N3A,N4B,N5A,N6A,N7B,N8A,N9A,N10A,N11A,N12A,N13A,N14A,N15B,N16B,N17A,N18B,N19A,N20A,N21A,N22A,N23B,N24B,N25B,N26B,N27A,N28B,N29B,N30B,N31B,N32B,N33B,N34B,N35B,N36A,N37A,N38A,N39B,N40A,N41A,N42A,N43B,N44A,N45B,N46A,N47A,N48A,N49B,N50B,N51B,N52A,N53B,N54B,N55B,N56B,N57A,N58A,N59A,N60B
GeneratedFlag=SUCTF{92d1c2c3f6e55fabbc3a6ffde57c7341}
```

这就已经把题目结论说明白了：

- 最优值是 `322`
- 最优路径数是 `1`
- 唯一最优路径已经被恢复出来

## 唯一路径

```text
N1B,N2B,N3A,N4B,N5A,N6A,N7B,N8A,N9A,N10A,
N11A,N12A,N13A,N14A,N15B,N16B,N17A,N18B,N19A,N20A,
N21A,N22A,N23B,N24B,N25B,N26B,N27A,N28B,N29B,N30B,
N31B,N32B,N33B,N34B,N35B,N36A,N37A,N38A,N39B,N40A,
N41A,N42A,N43B,N44A,N45B,N46A,N47A,N48A,N49B,N50B,
N51B,N52A,N53B,N54B,N55B,N56B,N57A,N58A,N59A,N60B
```

## Flag 生成逻辑

真结局后不是直接输出路线，而是取每个选项的 `marker`。

相关代码：

- [`GameManager.cs`](/e:/Unity%20down/esaygal/Assets/Scripts/Runtime/GameManager.cs)
- [`GameStateStore.cs`](/e:/Unity%20down/esaygal/Assets/Scripts/Runtime/GameStateStore.cs)
- [`FlagUtility.cs`](/e:/Unity%20down/esaygal/Assets/Scripts/Runtime/FlagUtility.cs)

流程：

1. 每次点击选项，记录该选项的 `marker`
2. 真结局时按顺序拼接全部 `marker`
3. 对拼接后的字符串做 `MD5`
4. 包成 `SUCTF{...}`

验证器已经直接把当前实例的结果算出来了：

```text
SUCTF{92d1c2c3f6e55fabbc3a6ffde57c7341}
```

## 一句话总结

这题现在的解法不是“试错”也不是“肉眼挑甜选项”，而是：

1. 从 [`story.json`](/e:/Unity%20down/esaygal/Assets/Resources/Story/story.json) 读出 `60` 个节点的 `weight/value`
2. 用 DP 算出 `w <= 132` 时的最大 `value`
3. 同时统计最优路径数
4. 确认 `OptimalPathCount = 1`
5. 取这条唯一路径对应的 `marker` 串做 `MD5`
6. 得到最终 flag
