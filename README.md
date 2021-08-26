## 开发手册
[PikaScript-三分钟快速上手](doc/1.%E4%B8%89%E5%88%86%E9%92%9F%E5%BF%AB%E9%80%9F%E4%B8%8A%E6%89%8B.md)

## stm32例程
[pikascript-stm32-demo](../../../pikascript-demo-stm32)

# 1.简介
pikascript可以为mcu裸机c开发提供面向对象的脚本绑定。

支持裸机运行，可运行于内存8Kb以上的mcu中，如stm32f103c8t6，esp32。


支持跨平台，可在linux环境开发、测试内核。

开箱即用，零配置，仅使用C标准库，几乎不使用宏，几乎不使用全局变量。

完整的单元测试。

栈空间少量占用，内存信息可观测。

尽可能的结构清晰（尽我所能）。

# 2.stm32教程视频

[手把手PikaScript教程合集](https://www.bilibili.com/video/BV1mg411L72e)

# 3.原理解析：

## 架构示意图
![K9C)%CZO)X`8VFBKCE8(9@W](https://user-images.githubusercontent.com/88232613/127806449-b476b2fd-9f40-4c53-94a0-e1e965c046c3.png)

## 内核

[PiKaScript 运行时内核](https://github.com/mimilib/pikascript-core)


## 预编译器

[pikascript-compiler-Rust](https://github.com/mimilib/pikascript-compiler-rust)

## 解析文章
[单片机运行python脚本，不移植micropython也行吗？](https://mp.weixin.qq.com/s?__biz=MzU4NzUzMDc1OA==&mid=2247484127&idx=1&sn=f66cff49c488e48c52570c7bb570328f&chksm=fdebd5b6ca9c5ca0707fd221c32f3ad63e94aeb6f917a92774b89ea042381ea261990f5cca3c&token=2045971639&lang=zh_CN#rd)

## 微信交流群
![微信图片_20210825171046](https://user-images.githubusercontent.com/88232613/130763024-c57106f5-0d46-43d8-99e3-c331ae2594b5.jpg)

## 官方公众号
![image](https://user-images.githubusercontent.com/88232613/128301451-f0cdecea-6457-4925-b084-42e7796a856e.png)

# 4.内核测试与开发

## linux下测试pikascript内核(推荐ubuntu20.04)：

step1: 拉取项目
``` shell
git clone https://github.com/mimilib/pikascript
cd PikaScript
```

step2: 编译项目
```
sh init.sh # 第一次编译前运行，之后就不用运行了 
sh make.sh # 编译项目
```

step3: 运行单元测试
``` shell	
sh test
```

step4: 运行基准测试
``` shell
sh test-banchmark
```

step5: 运行demo
``` shell
./build/src/boot/demo06-pikamain/pikascript_demo06-pikamain
```

step6: 提交修改
``` shell
sh commit.sh
```
