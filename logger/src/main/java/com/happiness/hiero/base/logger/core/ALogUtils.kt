package com.happiness.hiero.base.logger.core

import android.util.Log
import com.happiness.hiero.base.logger.interfaces.ILogger
import java.text.SimpleDateFormat
import java.util.*


/**
 * log for android
 */
val format: SimpleDateFormat = SimpleDateFormat("MM-dd HH:mm:ss SSS", Locale.CHINA)
object ALogUtils : ILogger {
    var logEnable = true

    private val timeTag: String
        get() = String.format("[%s] ", format.format(Date(System.currentTimeMillis())))

    fun write(tag: String?,formattedMsg:String){
        AppLogMgr.invokeMethod("$timeTag${tag?.intern()}${formattedMsg.intern()}\r".toCharArray())
    }

    override fun e(tag: String?, extra:String?, msg: String) {
        if (!logEnable) return
        val formattedMsg="[${extra?.intern()}][tid@${Thread.currentThread().id}(${Thread.currentThread().name.intern()})][$msg]".intern()
        write(tag, formattedMsg)
    }

    override fun w(tag: String?, extra:String?, msg: String) {
        if (!logEnable) return
        val formattedMsg="[${extra?.intern()}][tid@${Thread.currentThread().id}(${Thread.currentThread().name.intern()})][$msg]".intern()
        Log.w(tag, formattedMsg)
        write(tag, formattedMsg)
    }

    override fun d(tag: String?, extra:String?, msg: String) {
        if (!logEnable) return
        val formattedMsg="[${extra?.intern()}][tid@${Thread.currentThread().id}(${Thread.currentThread().name.intern()})][$msg]".intern()
        Log.d(tag, formattedMsg)
        write(tag, formattedMsg)
    }

    override fun i(tag: String?, extra:String?, msg: String) {
        if (!logEnable) return
        val formattedMsg="[${extra?.intern()}][tid@${Thread.currentThread().id}(${Thread.currentThread().name.intern()})][$msg]".intern()
        Log.i(tag, formattedMsg)
        write(tag, formattedMsg)
    }

    override fun v(tag: String?, extra:String?, msg: String) {
        if (!logEnable) return
        val formattedMsg="[${extra?.intern()}][tid@${Thread.currentThread().id}(${Thread.currentThread().name.intern()})][$msg]".intern()
        Log.v(tag, formattedMsg)
        write(tag, formattedMsg)
    }

    override fun trace(tag: String?, extra: String?, tr: Throwable) {
        if (!logEnable) return
        val formattedMsg="[${extra?.intern()}][tid@${Thread.currentThread().id}(${Thread.currentThread().name.intern()})][exception -> ${tr.message}]".intern()
        Log.e(tag, formattedMsg)
        write(tag, formattedMsg)
    }
}