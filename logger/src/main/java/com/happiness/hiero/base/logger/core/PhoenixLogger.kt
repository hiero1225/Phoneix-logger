package com.happiness.hiero.base.logger.core

import com.happiness.hiero.base.logger.interfaces.ILogger
import com.happiness.hiero.base.logger.interfaces.OnFileShrinkCallback

object PhoenixLogger: ILogger {

    init {
        System.loadLibrary("phoenix")
    }

    external fun init(path:String,cacheDir:String,logDir:String,maxFileSize:Int,onFileWriteCallback: OnFileShrinkCallback)
    external  fun uninit()
    external fun flush()
    external override fun e(tag: String?, extra: String?, msg: String)

    external override fun w(tag: String?, extra: String?, msg: String)

    external override fun d(tag: String?, extra: String?, msg: String)

    external override fun i(tag: String?, extra: String?, msg: String)

    external override fun v(tag: String?, extra: String?, msg: String)

    override fun trace(tag: String?, extra: String?, tr: Throwable) {

     }


}