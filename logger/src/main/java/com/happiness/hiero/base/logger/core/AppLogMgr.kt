package com.happiness.hiero.base.logger.core


import android.util.Log
import com.happiness.hiero.base.logger.Constants.MODULE_TAG
import com.happiness.hiero.base.logger.interfaces.OnFileShrinkCallback
import com.happiness.hiero.base.logger.utils.FileLogHelper
import com.happiness.hiero.base.logger.utils.FileLogHelper.write2Buffer
import java.io.File
import java.nio.CharBuffer
import java.util.concurrent.atomic.AtomicBoolean


/**
 * APP日志管理，网络请求错误的日志缓存到本地，用于故障诊断
 * 性能：5M/s压力-约等于缓冲区大小
 * 内存消耗：5(buffer)+5/2(back buffer)=7.5M
 */
object AppLogMgr {
    private const val TAG = "AppLogMgr"
    private const val MAX_CACHE_SIZE = 2 * 1024 * 1024
    var MAX_LOG_CAPACITY = 50 * 1024 * 1024
    var MAX_FILE_SIZE = 5 * 1024 * 1024

    @Volatile
    private lateinit var logDir: String
    private val mainBuff: CharBuffer = CharBuffer.allocate(MAX_CACHE_SIZE)
    private val tempBuff: CharBuffer = CharBuffer.allocate(MAX_CACHE_SIZE / 2)
    private val fileWriteTag: AtomicBoolean by lazy { AtomicBoolean(false) }
    private var init = false
    private var shrinkCallback: (() -> Unit)? = null
    private const val BREAK_IN_FLUSH = "-----------------flush-----------------\r\n\r\n"
    private var logFileName = "log.log"

    fun init(
        loggerType: LoggerType = LoggerType.PHOENIX_LOG,
        cacheDir: String,
        logDir: String,
        maxFileSize: Long = 0,
        logName: String? = null,
        shrinkCallback: (() -> Unit)? = null
    ) {
        logName?.run { logFileName = this }
        LoggerUtils.type = loggerType
        AppLogMgr.logDir =logDir
        AppLogMgr.shrinkCallback =shrinkCallback
        when (loggerType) {
            LoggerType.ALOG -> {
                init(logDir, logName, shrinkCallback)
            }

            LoggerType.PHOENIX_LOG ->{
                initNative(logFileName,cacheDir,logDir, MAX_FILE_SIZE)
            }
        }
    }

    /**
     * @param mLogPathDir 自定义路径
     * @param shrinkCallback 备份日志回调
     */
    fun init(
        mLogPathDir: String,
        logName: String? = null,
        shrinkCallback: (() -> Unit)? = null
    ) {
        logDir = mLogPathDir
        LoggerUtils.logger=ALogUtils
        logName?.run { logFileName = this }
        AppLogMgr.shrinkCallback = shrinkCallback
        init = true
        Log.d(MODULE_TAG, "init app log dir=$logDir")
    }



    private fun initNative(logName: String, cacheDir: String, logDir: String, maxFileSize: Int){
        cacheDir.run { File(this) }.run { mkdirs() }
        logDir.run { File(this) }.run { mkdirs() }
        LoggerUtils.logger=PhoenixLogger
        PhoenixLogger.init(logName, cacheDir, logDir, maxFileSize, object : OnFileShrinkCallback {
            override fun onShrink(bmkPath: String) {
                Log.e(MODULE_TAG, "on shrink file with path $bmkPath ")
            }
        });
    }



    /**
     * should invoke this method only by logutils .flush() to keep the agent mechanism works
     */
    fun flush() {

        if (LoggerUtils.logger == PhoenixLogger) {
            PhoenixLogger.flush()
            return
        }

        if (LoggerUtils.logger != ALogUtils) return
        Thread {
            ALogUtils.d(MODULE_TAG, TAG, "flush all logs with tid:" + Thread.currentThread().id)
            val ts = System.currentTimeMillis()
            if (mainBuff.position() > 0) {
                Log.d(MODULE_TAG, " flush file:$logFileName")
                write2BufferInner(BREAK_IN_FLUSH.toCharArray(), true)
            }
            ALogUtils.d(MODULE_TAG, TAG, "flush all logs with tid:" + Thread.currentThread().id + ",flush time:" + (System.currentTimeMillis() - ts) + "ms")
        }.start()
    }


    private fun write2BufferInner(value: CharArray, writeImmediately: Boolean) {
        //save to buffer
        if (!write2Buffer(mainBuff, tempBuff, value, writeImmediately)) {
            if (fileWriteTag.get()) {
                Log.i(MODULE_TAG, "flush stop when writing: ${fileWriteTag.get()}")
                return
            }
            fileWriteTag.set(true)
            Log.i(MODULE_TAG, "$logFileName is going to write with buffer size ${mainBuff.position()},logDir:$logDir,tid ${Thread.currentThread().id}")
            File(logDir).takeIf { !it.exists() }?.apply { this.mkdirs() }
            val file = File(logDir, logFileName)
            Thread {
                FileLogHelper.write2File(file, mainBuff) {
                    if (file.length() > MAX_FILE_SIZE) {
                        Log.d(MODULE_TAG, file.name + "  file length > MAX_SIZE,do file split action " + Thread.currentThread().id)
                        FileLogHelper.shirkLogWithTimeRolling(logDir, file, MAX_LOG_CAPACITY - MAX_FILE_SIZE, MAX_FILE_SIZE)
                    }
                }

                synchronized(tempBuff) {
                    Log.e(MODULE_TAG, "lock temp buff(pos${tempBuff.position()}) ,tid ${Thread.currentThread().id}")
                    val pos = tempBuff.position()
                    tempBuff.put(tempBuff.array(), 0, pos)
                    tempBuff.clear()
                    Log.e(MODULE_TAG, "unlock temp buff,re-assign $pos to buffer,tid ${Thread.currentThread().id}")
                }
                fileWriteTag.set(false)
            }.start()
        }
    }


    /**
     * 事件处理，需要记录到内存或者写入日志
     * @param value 数据
     * todo tag 分开写入，buffer缓冲加大，是否因为synchronized 导致主线程anr?
     */
    fun invokeMethod(value: CharArray) {
        if (!init) {
            Log.e(TAG,"not init!!!!")
            return
        }
        write2BufferInner(value, false)
    }

    fun relalse(){
        if( LoggerUtils.type == LoggerType.PHOENIX_LOG){
            PhoenixLogger.uninit()
        }
    }

}