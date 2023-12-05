日志框架净化之路

* 关键词：mmap,java,双缓冲，c++.,ndk,zstd,压缩

* 需求背景：
日志需要保存到本地，并在需要时进行上传

topic1:传统java日志框架怎么做？

从java层做需要考虑到如下情况：
1.日志string如何存到内存中，总不能来一条日志写一条吧？这样会导致频繁IO,是非常愚蠢的策略。所以我们不考虑使用Hashmap或者arraylist来存储每一条日志，为了减少IO，我们的方案是ByteArrayBuffer.

2.多线程高并发下如何写入的问题，需要用到锁和buffer并且是双buffer.一个Buffer在往磁盘写入的时候如果有日志过来这个时候需要写入备份的buffer中。在主buffer flush到磁盘之后把备份buffer flush到主buffer.

以上方案我们实现了ALogger，提供了基于接口的封装

3.但是新问题来了？进程崩溃了我们的缓冲区来不及写入，会导致日志丢失。 另外，高并发的场景，特别是在类似生产消费者模型中长时间的while循环里面写入高并发日志会产生大量的临时string驻留在内存中从而造成内存抖动，特别业务层还有很多格式化日志比如加入时间日志等场景会导致情况更糟。但你手动GC是可以回收的。但是这对于我们来说是不可接受的。


* 由此引发的问题：我们怎么优化内存，怎么在一个长时间while循环里面高并发的场景下去做无抖动低内存的日志框架方案？ 答案是：mmap ,java层已经没有能力解决这个问题。


谈一谈mmap:

1.原理：MMAP大家应该非常熟悉，我们在安卓开发中熟悉的Binder机制就是利用mmap在两个进程中开辟一块内存区域，一个进程写入之后只需要从用户空间拷贝到这个映射的内存区域，那么另外一个进程就能通过这个区域读取到数据。这就是一次拷贝的本质。

它依赖的是linux的内存管理机制，虚拟内存，把两个进程的地址空间映射到同一块物理内存区域。

2.它为什么能解决崩溃场景日志丢失问题：

 mmap开辟的内存是操作系统维护的，我们的日志写入，直接开辟一块内存区域吧这个区域与一个fd绑定，fd对应磁盘的一个文件句柄。 如果进程崩溃，操作系统会做一个类似flush操作。简单而言，它不像java那样需要你主动flush,它的控制权在OS.所以你崩了Os就会在这个时机帮你flush.另外你如果主动unmmap的话也会flush.

3.它为什不会产生内存抖动？ 很简单:c/c++内存管理和释放是我们自己控制的，而java它是gc 线程定时扫描达到一定阈值才会去回收。


4.mmap怎么使用？

上代码，直接干：

int8_t *mmap(int fd, int pageStep) {
auto *result = static_cast<int8_t *>(mmap(nullptr, PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, pageStep * PAGE_SIZE));
return result;
}

注意，mmap申请以页为单位，这涉及到操作系统内存管理的知识，只用记住一个页是4096个字节即可。我们申请一个page足够了。如果是java层，都是以M为单位了。 另外，最后一个参数需要注意，必须是PAGE的整数倍。mmap这个方法返回的是一个指针地址，表示你申请的内存的首地址。


5.实现思路：日志写入加锁，把字符串格式化之后放到这个mmap申请的内存中，每次放入字符串之后，地址往后挪动，挪动的大小就是你字符串的长度。最后挪动的位置超过一个PAGE容器的大小的时候就把做unmmap映射到磁盘中去。如果字符串超过了大小要做拆分和递归，确保一个page被填满。


6.坑点：注意这里有坑，有天坑。
   
  很多平台对于磁盘的读写是需要权限的，比如安卓平台，很多情况下日志的写入会先于用户的权限申请和同意。那么你这个mmap甚至都没办法申请，因为你映射的那个文件在外部存储中。作为一个优秀的日志框架，这些必须考虑。基于此我们设计了类似于java层日志解决方案的双缓冲策略。在我们应用自己data分区搞一个缓冲文件，这个分区是不要权限的。

所有日志都从这类中转，满了之后中转到sdcard目录。等你一个Page的日志写满，权限的申请和授予早就结束了。


7. 日志如何压缩？ 解决方案：zstd,优势非常明显。 大家自行百科。 另外做一个一个优秀的日志框架，应该要考虑日志的滚动压缩策略，甚至加解密等。总之安全，空间资源都是有限的。本次我完成了日志的滚动压缩和总大小限制。但加解密暂时未实现，且大部分业务并不需要这么高级的日志功能，脱敏是另外一个话题。


8.性能

我对比了腾讯的mars-log框架，和我自己实现的phoenix-log框架以及本框架内置的java层日志解决方案的性能数据，两个线程同时执行，每个线程执行10w次日志写入操作.before 代表native内存和java堆内存的初始化状态的值， 数据如下

    /**
     * 测试数据：20w lines ,25M文本,单位为兆
     * performance: native heap vs java heap
     * PHONEIX:
     *  before:10 vs 7.7
     *  after:native->10.5,java->14.4
     *  GC:8，日志不丢失，实时性好，IO可以忽略,跨平台,压缩率更高1/500
     *
     *    before:6.9 vs 7.7
     *    after:native->7.8,java->11.4
     *
     *
     * JAVA:
     *   before:9.7 vs 7.8
     *   after:native->10,java->15
     *   GC: 不计其数 内存剧烈抖动，频繁GC ，日志丢失，实时性较差，有IO，仅支持JAVA
     *
     *  MARS:
     *     before:9.2 vs 8.1
     *     after:native->11.6,java->14.7
     *     GC:8，日志不丢失，实时性未确认，IO可以忽略，跨平台

     *      10w
     *        before:7.5 vs 7.8
     *        after:native->9.9,java->11.4
     *
     */



测试代码：

    val TAG = "MainActivity"
    val testString = "fadfsadsaffaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaafsa问"
    private fun writeLogs(tag: String) {
        thread(name = tag) {
            var i = 0
            while (i < 500000 && !finish) {
                i++
                LoggerUtils.v("BX-TEST", TAG, "$i->$testString")
            }

            //崩溃测试代码，非主线程操作UI那肯定是要崩溃的，注意观察日志日周写入
            Toast.makeText(this,"finished",Toast.LENGTH_SHORT).show()
        }
    }

    private fun writeLogs2(tag: String) {
        thread(name = tag) {
            var i = 0
            while (i < 50000 && !finish) {
                i++
                LoggerUtils.v("BX-TEST-BOY", TAG, "$i->$testString")
            }
            runOnUiThread{
                Toast.makeText(this,"finished",Toast.LENGTH_SHORT).show()
            }

        }
    }

    private fun writeExceptionLogs(tag: String) {
        thread(name = tag) {
            try {
                1 / 0
            } catch (e: Exception) {
                LoggerUtils.e("BX-TEST-ERROR", TAG, e.message+"")
                return@thread }

        }
    }



综上：性能上跟腾讯的mars差别不大(别问我为啥不用mars因为要自主可控轻量，mars固然很好但我有洁癖，它的公共库有很多我不需要的功能),跟java框架下的性能优势更是云泥之别。堪称完美，哈哈哈哈。 关于项目的源代码我放到了github上，欢迎大家加星点赞，希望大家在使用的时候尊重别人的劳动成果，本项目如果是安卓可以直接拿过去用，如果是PC或者Ios请自行交叉编译。 

另外：大家放心使用，基本上拿过来就能用，目前没有发现任何bug.有bug的话大家留言 ，上面的cmake是写好了的，如果不想编译也不想写封装代码，加我qq发aar可以直接调用。



FAQ:
1.用法：
 step1 ：
 AppLogMgr.init进行初始化
 第一个参数是日志类型（ android基于java buffer的原生）,PHONEIX_LOG（mmap方案）两种，老铁们自己选择用那种，建议：如果没有特殊业务需求，Alog即可满足日常需求。

 step2：
 日志调用 LoggerUtils.xx方法即可


 调用示例：
 AppLogMgr.init(LoggerType.PHOENIX_LOG, app.filesDir.path,  app.filesDir.path,)
//日志最大容量为50M
AppLogMgr.MAX_FILE_SIZE = 5 * 1024 * 1024
LoggerUtils.i(HomeModule.MODULE_TAG, clzTag, "init with env: ${InfoUtils.getFormattedItemsInfo(app)}")


















