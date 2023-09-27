package com.happiness.hiero.base.logger.interfaces

interface ILogger {

    fun e(tag: String?, extra:String?, msg: String)

    fun w(tag: String?, extra:String?, msg: String)

    fun d(tag: String?,extra:String?, msg: String)

    fun i(tag: String?, extra:String?, msg: String)

    fun v(tag: String?, extra:String?, msg: String)

    fun trace(tag: String?, extra:String?, tr: Throwable)
}


interface OnFileShrinkCallback{
    fun onShrink(bmkPath: String)
}