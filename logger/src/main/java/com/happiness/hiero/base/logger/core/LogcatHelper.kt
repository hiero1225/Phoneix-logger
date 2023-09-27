package com.happiness.hiero.base.logger.core


import java.io.*
import java.text.SimpleDateFormat
import java.util.*

/**
 * Created by hierophantzhang on 2017/2/17.
 * logcat dumper for adb cmd
 */
@Deprecated("d")
class LogcatHelper private constructor() {
    private var mLogDumper: LogDumper? = null
    private var sb: StringBuffer? = null
    private val simpleDateFormat2 = SimpleDateFormat("yyyy-MM-dd HH:mm:ss", Locale.CHINA)
    fun start() {
        if (mLogDumper != null) mLogDumper!!.stopLogs()
        mLogDumper = null
        mLogDumper = LogDumper()
        mLogDumper!!.start()
    }

    fun stop() {
        if (mLogDumper != null) {
            mLogDumper!!.stopLogs()
            mLogDumper = null
        }
    }

    private inner class LogDumper : Thread() {
        private var logcatProc: Process? = null
        private var mReader: BufferedReader? = null
        private var mRunning = true
        private var out: FileOutputStream? = null
        fun stopLogs() {
            mRunning = false
        }

        override fun run() {
            synchronized(this) {
                try {
                    val logcatProc = Runtime.getRuntime().exec(cmds)
                    val file = File(DIR, logFileName!!)
                    //clear oldfile
                    file.delete()
                    file.createNewFile()
                    out = FileOutputStream(file)
                    sb = StringBuffer()
                    mReader = BufferedReader(InputStreamReader(logcatProc.inputStream), 1024)
                    var line: String? = null
                    while (mRunning && mReader!!.readLine().also { line = it } != null) {
                        if (!mRunning) {
                            break
                        }
                        if (line?.length == 0) {
                            continue
                        }
                        if (out != null && line?.contains(mPId.toString() + "") == true) {
                            val logstr = """${simpleDateFormat2.format(Date())}  $line
"""
                            sb!!.append(logstr)
                            // out.write((logstr.getBytes());
                        }
                    }
                } catch (e: IOException) {
                    e.printStackTrace()
                } finally {
                    if (logcatProc != null) {
                        logcatProc!!.destroy()
                        logcatProc = null
                    }
                    if (mReader != null) {
                        try {
                            mReader!!.close()
                            mReader = null
                        } catch (e: IOException) {
                            e.printStackTrace()
                        }
                    }
                    if (out != null) {
                        try {
                            out!!.write(sb.toString().toByteArray())
                            out!!.flush()
                            out!!.close()
                        } catch (e: IOException) {
                            e.printStackTrace()
                        }
                        out = null
                    }
                }
            }
        }
    }

    companion object {
        var mPId = 0
        private var INSTANCE: LogcatHelper? = null
        private var DIR: String? = null
        private var cmds: String? = null
        private var logFileName: String? = null
        val instance: LogcatHelper?
            get() {
                if (INSTANCE == null) {
                    INSTANCE = LogcatHelper()
                }
                return INSTANCE
            }

        fun init(dir: String?, filename: String?) {
            mPId = android.os.Process.myPid()
            logFileName = filename
            DIR = dir
            val dirF = DIR?.let { File(it) }
            if (dirF != null && !dirF.exists()) {
                dirF.mkdirs()
            }
            //        if (!dirF.isDirectory())
            //            throw new IllegalArgumentException("The logcat folder path is not a directory: " + dirF);
            cmds = "logcat *:e *:i | grep \"($mPId)\""
        }
    }
}