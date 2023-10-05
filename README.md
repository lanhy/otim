# otim(open tars IM)
## 作者简介
    兰怀玉 
    毕业于中央民族大学计算机专业
    先后供职国内外多家公司软件研发设计岗位，有丰富的软件研发经验。
    从事IM领域设计研发十余年，先后领衔多款IM通讯系统设计与研发，有非常丰富的IM系统设计研发经验。
    
## 基于Tars高并发IM系统的设计与实现
主要介绍基于Tars框架的IM服务系统开发，IM是一个要求很高的高频低延时系统，要求越高，研发门槛自然就越高； 
将内容分基础篇，进阶篇，实战篇三部分逐步刨析高性能高并发IM系统设计与实现。

作者经过多年IM系统开发设计工作，对IM的技术体系有丰富经验，将IM体系架构总结为一句话“IM技术一二三四五”；
具体如下:
* 一个通讯协议
* 两个架构
    * 客户端端
    * 服务端端
* 三大指标
    * 高可用
    * 高并发
    * 低延时
* 四大模块
    * 连接管理
    * 用户及好友管理
    * 消息管理
    * 离线推送
* 五大难题
    * 连接稳定
    * 消息一致性
    * 历史消息
    * 未读数
    * 超大群

主要针对以上5个方面进行详细阐述，一步一步教您如何构建一个高并发IM系统。


## 目录结构：

- doc 
    - [1-基础篇 基于Tars高并发IM系统的设计与实现](https://github.com/lanhy/otim/blob/main/doc/1-%E5%9F%BA%E7%A1%80%E7%AF%87%20%E5%9F%BA%E4%BA%8ETars%E9%AB%98%E5%B9%B6%E5%8F%91IM%E7%B3%BB%E7%BB%9F%E7%9A%84%E8%AE%BE%E8%AE%A1%E4%B8%8E%E5%AE%9E%E7%8E%B0.md)
    - [2-进阶篇 基于Tars高并发IM系统的设计与实现](https://github.com/lanhy/otim/blob/main/doc/2-%E8%BF%9B%E9%98%B6%E7%AF%87%20%E5%9F%BA%E4%BA%8ETars%E9%AB%98%E5%B9%B6%E5%8F%91IM%E7%B3%BB%E7%BB%9F%E7%9A%84%E8%AE%BE%E8%AE%A1%E4%B8%8E%E5%AE%9E%E7%8E%B0.md)
    - [3-实战篇 基于Tars高并发IM系统的设计与实现](https://github.com/lanhy/otim/blob/main/doc/3-%E5%AE%9E%E6%88%98%E7%AF%87%20%E5%9F%BA%E4%BA%8ETars%E9%AB%98%E5%B9%B6%E5%8F%91IM%E7%B3%BB%E7%BB%9F%E7%9A%84%E8%AE%BE%E8%AE%A1%E4%B8%8E%E5%AE%9E%E7%8E%B0.md)
    - [《OMTP协议说明文档》](https://github.com/lanhy/otim/blob/main/doc/OMTP%E5%8D%8F%E8%AE%AE%E8%AF%B4%E6%98%8E%E6%96%87%E6%A1%A3.md)
   
- client 
    - 客户端SDK及示例代码
- server 
    - 服务端代码，基于Tars微服务架构的IM系统相关子服务；
- test 
    - 自动测试代码;

## CSDN专栏
[IM技术](https://blog.csdn.net/lanhy999/category_12364971.html)


## 鸣谢
在本系列文章及本系统开源代码编写过程中，有一些朋友提供了部分内容，代码及宝贵意见，在此表示诚挚感谢。

特别鸣谢以下朋友：
* 郝永建老师
* 张昌海
* 王志永
* 杨亮
* 王进

欢迎大家提供宝贵意见及建议，QQ交流群:935207532 。
