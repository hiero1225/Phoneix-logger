package com.happiness.hiero.base.logger.core

import com.happiness.hiero.base.logger.interfaces.ILogger


object LoggerUtils : ILogger {

    var type: LoggerType = LoggerType.ALOG

    var logger: ILogger =ALogUtils
        get() {
            return when (type) {
                LoggerType.ALOG -> ALogUtils
                LoggerType.PHOENIX_LOG -> PhoenixLogger
            }
        }

    override fun e(tag: String?, extra: String?, msg: String) {
        logger.e(tag, extra, msg)
    }

    override fun w(tag: String?, extra: String?, msg: String) {
        logger.w(tag, extra, msg)
    }

    override fun d(tag: String?, extra: String?, msg: String) {
        logger.d(tag, extra, msg)
    }

    override fun i(tag: String?, extra: String?, msg: String) {
        logger.i(tag, extra, msg)
    }

    override fun v(tag: String?, extra: String?, msg: String) {
        logger.v(tag, extra, msg)

    }

    override fun trace(tag: String?, extra: String?, tr: Throwable) {
        logger.trace(tag, extra, tr)
    }
}






enum class LoggerType {
    ALOG,
    PHOENIX_LOG,
}