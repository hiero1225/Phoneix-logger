package com.happiness.hiero.base.logger.base

import com.happiness.hiero.base.logger.core.LoggerUtils


open class Logger(private val tag: String, private val clz: String? = null) {

    open fun logDebug(msg: String) {
        LoggerUtils.d(tag, clz ?: this.javaClass.simpleName, msg)
    }

    open fun logVerbose(msg: String) {
        LoggerUtils.v(tag, clz ?: this.javaClass.simpleName, msg)
    }

    open fun logInfo(msg: String) {
        LoggerUtils.i(tag, clz ?: this.javaClass.simpleName, msg)
    }

    open fun logWarn(msg: String) {
        LoggerUtils.w(tag, clz ?: this.javaClass.simpleName, msg)
    }

    open fun logError(msg: String) {
        LoggerUtils.e(tag, clz ?: this.javaClass.simpleName, msg)
    }
}