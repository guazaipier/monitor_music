1. qt creator 6.8.3 中编写的，生成的 exe 文件搭配 config.ini 文件即可运行。
2. config.ini 文件为本地 mysql 数据库的配置信息，需要根据实际情况修改。
3. 运行前，需要确保 mysql 数据库已经启动，程序运行后会在该数据库中自动创建四个数据表，分别为：
    - likelist_tb 喜欢的音乐列表
    - localist_tb 本地音乐列表 （暂时没有用到，但是保留这个表）
    - historylist_tb 历史播放列表
    - playlist_tb 当前播放列表
4. TODO: 界面刚打开有点卡，是由于在加载网络图标


Notice：里面用的音乐 api 资源来自于该网站，仅用于学习，不会用来商用
