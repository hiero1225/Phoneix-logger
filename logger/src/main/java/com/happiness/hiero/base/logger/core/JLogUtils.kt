package com.happiness.hiero.base.logger.core

import com.happiness.hiero.base.logger.interfaces.ILogger

/**
 * log for java
 */

object JLogUtils : ILogger {

    override fun e(tag: String?, extra:String?, msg: String) {
        println("[$tag][${extra?:javaClass.simpleName}][$msg]")
    }

    override fun w(tag: String?, extra:String?, msg: String) {
        println("[$tag][${extra?:javaClass.simpleName}][$msg]")
    }

    override fun d(tag: String?, extra:String?, msg: String) {
        println("[$tag][${extra?:javaClass.simpleName}][$msg]")
    }

    override fun i(tag: String?, extra:String?, msg: String) {
        println("[$tag][${extra?:javaClass.simpleName}][$msg]")
    }

    override fun v(tag: String?, extra:String?, msg: String) {
        println("[$tag][${extra?:javaClass.simpleName}][$msg]")
    }

    override fun trace(tag: String?, extra: String?, tr: Throwable) {
        println("[$tag][${extra?:javaClass.simpleName}][exception -> ${tr.message}]")
    }
}