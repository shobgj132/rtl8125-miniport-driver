# RTL8125 Custom Windows Miniport Driver

自主开发的 Realtek RTL8125 / RTL8125B **2.5GbE** Windows NDIS Miniport 内核驱动。

专为 **高吞吐、低延迟、DMA 网络传输** 场景设计。适配 **PCILeech** 生态，支持 **FPGA DMA 板卡**（Squirrel / Enigma X1 / CaptainDMA 75T / 35T 等）通过 **pmemtcp 协议** 经网络高速回传 DMA 读取数据，同时也可作为通用高性能 2.5GbE 网卡驱动独立使用。

## 支持芯片

| 芯片型号 | PCIe Device ID | 硬件修订版本 | PHY Firmware | 说明 |
|----------|---------------|-------------|-------------|------|
| **RTL8125A** | `10EC:8125` | Rev 00 - 02 | — | 初代 2.5GbE，MSI/MSI-X，多队列 |
| **RTL8125B** | `10EC:8125` | Rev 03 - 05 | 内置 RAM Code | 主流版本，扩展 L1 Off，PHY 调优 |
| **RTL8125BP** | `10EC:8125` | Rev 06 - 09 | 内置 RAM Code | B 系列改良版，新中断合并引擎（v6），新 RX 描述符格式 |
| **RTL8125D** | `10EC:8125` | Rev 0A - 0D | 内置 RAM Code + PHY Patch | 新一代，需 PHY firmware patch 才能稳定工作 |
| **RTL8125CP** | `10EC:8125` | Rev 0E+ | 内置 RAM Code | 紧凑型封装变体 |

> INF 覆盖 Rev 00 ~ Rev 1F 全部 32 个硬件修订版本。驱动运行时自动识别芯片型号，加载对应的 PHY firmware 和硬件配置策略。

## 架构

```mermaid
graph LR
    A["🔧 FPGA DMA 板卡<br/>Squirrel / Enigma X1 / 75T"] -->|"PCIe DMA<br/>物理内存读写"| B["🖥️ 目标机"]
    B -->|"TCP/IP<br/>pmemtcp 协议"| C["📡 RTL8125 网卡<br/>本驱动"]
    C -->|"2.5GbE<br/>高速回传"| D["💻 分析机<br/>LeechCore / MemProcFS"]
```

```
目标机内部数据流：

  应用层    pmemtcp 服务端（WSK 内核套接字）
              │
              ▼
  传输层    TCP/IP 协议栈（Windows 内核）
              │  组装 NET_BUFFER_LIST（可能 1400+ 个 NB）
              ▼
  驱动层    RTL8125 Miniport Driver（本项目）
              │  NBL 拆分 → SG DMA / 线性化 → 描述符环写入
              ▼
  硬件层    RTL8125B 网卡（PCIe Bus Master DMA → 2.5GbE PHY → 网线）
```

## 为什么做这个

FPGA DMA 方案（PCILeech / LeechCore）做物理内存读写时，数据需要通过网络回传到分析机。这条链路的瓶颈往往不在 FPGA 端，而在 **网卡驱动的发送性能**：

- 官方 RTL8125 驱动不是为持续高吞吐 DMA 回传设计的
- 大量小包 + 高频发送容易触发丢包、排队超时、ring 溢出
- 出问题时没有诊断手段，黑盒调试效率极低

所以从零写了这个驱动，完全掌控 **PCIe DMA 映射、TX/RX 描述符环、中断调度、内存管理** 每一层。

## 核心技术

### DMA 与数据通路

- **Scatter-Gather DMA** — 硬件直接读写物理分散的内存页，减少 CPU 拷贝开销
- **Bus Master DMA** — PCIe 总线主控传输，网卡直接访问系统物理内存
- **TX Ring Descriptor 管理** — 1024 条描述符环，Producer/Consumer 模型，硬件直接消费
- **NBL 拆分发送** — TCP 栈下发的超大 NET_BUFFER_LIST（1400+ 个 NB）自动按描述符预算拆分，引用计数确保零丢包
- **发送排队 + 背压控制** — Ring 满时自动排队，内联回收 + DPC 回收 + 超时兜底，FIFO 保序
- **早期线性化 Fast Path** — 小包跳过 SG 分配，直接拷贝到连续 DMA buffer，消除 SG 分配失败

### 中断与调度

- **MSI / MSI-X 中断** — PCIe 消息信号中断，低延迟，无共享冲突
- **NDIS DPC 调度** — 中断 → Deferred Procedure Call → 批量处理收发完成，减少上下文切换
- **中断合并（Interrupt Coalescing）** — 可调节合并策略，平衡吞吐与延迟
- **TX / RX 独立中断统计** — 精确定位瓶颈在发送侧还是接收侧
- **Timer-based Polling** — 补偿中断丢失场景，保证数据路径不停滞

### PCIe / PHY / MAC 硬件层

- **PCIe Configuration Space 访问** — BAR 映射、Capability 解析、电源状态管理
- **PHY Firmware 加载** — RTL8125B / RTL8125D RAM Code 嵌入驱动，POR 时自动写入 PHY
- **PHY Patch 版本检查** — 读取当前 firmware 版本，避免重复写入，保护硬件
- **MDIO 寄存器读写** — PHY 层寄存器直接操作，配置自协商、速率、双工
- **MAC 寄存器完整配置** — 从硬件 reset 到 link up 全流程自主控制
- **Link State Debounce** — 链路状态抖动过滤，防止频繁 reset 导致连接中断

### 内存管理

- **NDIS Shared Memory 分配** — DMA 可达的连续物理内存，用于描述符环和数据缓冲区
- **TLB 友好的内存布局** — 描述符区域、Linear Buffer、Padding Region 页对齐连续分配
- **Physical / Virtual 地址映射** — 驱动内部维护物理地址表，硬件直接使用物理地址 DMA
- **MDL（Memory Descriptor List）处理** — 正确处理 CurrentMdlOffset、多 MDL 链、分页边界

### Offload 卸载

- **Checksum Offload** — TCP / UDP / IPv4 校验和计算交给网卡硬件
- **Large Send Offload (LSO v1 / v2)** — 大 TCP 段分片交给硬件，降低 CPU 负担
- **IP / TCP Header 解析** — 驱动内部解析协议头，正确设置硬件 offload 描述符

### 诊断与调试

- **50+ 项实时计数器** — xmit-ok / xmit-error / sg-alloc-failures / linearized-pkts / interrupt-count / dpc-count / send-timeout-count ...
- **配套命令行诊断工具** — 一条命令查看全部收发状态、中断统计、错误计数
- **Debug 版本 DebugView 日志** — 每个发送失败点都有 `RTL8125_TXDIAG:` 前缀，可用 DebugView 实时抓取
- **Send Timeout 检测 + 自动 Reset** — 4 秒超时 + 双重确认机制，防止驱动假死
- **Per-packet 错误追踪** — SG 分配失败、描述符溢出、线性化失败均有独立计数器

## 适用场景

- **PCILeech / FPGA DMA** — 配合 Squirrel、Enigma X1、CaptainDMA 75T/35T 等 FPGA 板卡，通过 pmemtcp 协议高速回传 DMA 读取数据
- **LeechCore / MemProcFS** — 作为 Network DMA 传输层，支持远程物理内存读写
- **高吞吐网络应用** — 需要持续稳定 2.5Gbps 发送能力的场景
- **嵌入式 / 定制 Windows 系统** — 需要自主可控网卡驱动的场景
- **安全研究 / 取证分析** — 物理内存采集与网络回传

## 测试数据

同一台 Windows 10 主机，双网卡，同一局域网，相同测试软件：

| 指标 | RTL8125（本驱动） | Intel I225-V（原厂驱动） |
|------|-------------------|------------------------|
| 持续速率 | 4200 - 4500 | 6000 - 6170 |
| 丢包（500 万包） | 0 | 0 |
| OutboundPacketErrors | 14（启动瞬态） | 0 |
| 发送路径 | 全线性化 | SG DMA |

> 性能差距主因已定位：当前 TX 线性化策略过于保守，Scatter-Gather DMA 路径未充分利用。优化中。

## 技术栈

`Windows Kernel` `NDIS 6.x` `WDM` `WDF` `PCIe` `DMA` `Bus Master` `Scatter-Gather` `MSI-X` `MMIO` `2.5GbE` `RTL8125B` `PHY Firmware` `MDIO` `Ring Buffer` `Descriptor Ring` `Interrupt Coalescing` `DPC` `LSO` `Checksum Offload` `TLB` `MDL` `PCILeech` `LeechCore` `pmemtcp` `FPGA DMA`

## 状态

持续开发中。核心收发已稳定，性能优化进行中。

## 联系

如果你对这个驱动感兴趣，或者有 FPGA DMA / Network DMA / Windows 内核驱动的定制需求：

<!-- 联系方式 -->
QQ 群 1092101318