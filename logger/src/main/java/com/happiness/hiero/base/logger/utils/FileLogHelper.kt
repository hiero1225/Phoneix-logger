package com.happiness.hiero.base.logger.utils

import android.util.Log
import com.happiness.hiero.base.logger.Constants.MODULE_TAG
import com.happiness.hiero.base.logger.core.LoggerUtils
import java.io.File
import java.io.FileInputStream
import java.io.FileNotFoundException
import java.io.FileOutputStream
import java.io.FileWriter
import java.io.IOException
import java.nio.CharBuffer
import java.nio.channels.Channels
import java.text.SimpleDateFormat
import java.util.Date
import java.util.Locale
import java.util.zip.Deflater.BEST_COMPRESSION
import java.util.zip.ZipEntry
import java.util.zip.ZipOutputStream
import kotlin.concurrent.thread

object FileLogHelper {
    private const val TAG = "FileUtils"
    private val dateFormat = SimpleDateFormat("yyyyMMdd HH_mm_ss", Locale.CHINA)

    fun shirkLogWithTimeRolling(dir: String, logFile: File, totalSize: Int, maxFileSize: Int, cutMode: CutMode = CutMode.STRICT) {
        trimLogDir(dir, totalSize, "-log.zip")
        val zipLogName = "${dateFormat.format(Date())}-${System.currentTimeMillis()}-log.zip"
        LoggerUtils.i(MODULE_TAG, TAG, "[shirkLogWithTimeRolling()]shrink file=${logFile.name},len=${logFile.length()}")
        try {
            val zipFile = File(dir, zipLogName)
            zipFile.createNewFile()
            val fileLength=logFile.length()
            if(fileLength-maxFileSize<=0){
                zipFile(logFile, maxFileSize, zipFile)
                Log.e( TAG,"invalid para $fileLength vs $maxFileSize")
            }else{
                shrinkFile(logFile, zipFile, maxFileSize, cutMode)
            }
        } catch (e: IOException) {
            e.printStackTrace()
            LoggerUtils.i(MODULE_TAG, TAG,"[shirkLogWithTimeRolling()]$zipLogName, error=${e.message}")
        }
    }

    private fun shrinkFile(file: File, zip: File, maxFileSize: Int, cutMode: CutMode = CutMode.STRICT) {
        Log.e( TAG, "[shrinkFile()]shrink file=${file.name},len=${maxFileSize}, cutMode=${cutMode}")
        try {
            val fileLength = file.length()
            val fileRename = File(file.parent, "_${file.name}").apply { if (exists()) delete() }
                .apply { createNewFile() }

            when (cutMode) {
                CutMode.STRICT -> {
                    //write backup file
                    zipFile(file, maxFileSize, zip)
                    val newFileWriter = FileOutputStream(fileRename, true)
                    val fis = FileInputStream(file)
                    val fisChannel = fis.channel
                    fisChannel.transferTo(
                        maxFileSize.toLong(),
                        fileLength - maxFileSize,
                        newFileWriter.channel
                    )
                    fisChannel.close()
                    fis.close()
                    newFileWriter.close()
                    file.delete()
                    val renameFile = File(file.parent, file.name)
                    fileRename.renameTo(renameFile)
                }

                CutMode.NORMAL -> {
                    file.renameTo(fileRename)
                    thread {
                        zipFile(fileRename, fileRename.length().toInt(), zip)
                        fileRename.delete()
                    }
                    if (!file.exists()) {
                        file.createNewFile()
                    }
                }
            }

        } catch (e: IOException) {
            e.printStackTrace()
            LoggerUtils.i(MODULE_TAG, TAG,"[shrinkFile()]error=${e.message}")
        }
    }

    @Synchronized
    fun trimLogDir(logDir: String, maxCap: Int, pattern: String, delimiters: String = "-") {
        var logCap = 0L
        val logList = HashMap<Long, File>()
        File(logDir).listFiles()?.forEach {
            if (it.name.endsWith(pattern)) {
                logCap += it.length()
                val fileTs = it.name.substringBefore(pattern).split(delimiters)[1].toLong()
                logList[fileTs] = it
            }
        }
        var spaceSize = maxCap - logCap
        val list = logList.keys.sorted()
        var index = 0
        var temp: File
        while (spaceSize < 0 && index < logList.size) {
            temp = logList.remove(list[index])!!
            spaceSize += temp.length()
            temp.delete()
            index++
            Log.e(MODULE_TAG, "space($spaceSize) vs logCap($logCap), deleted old log -> ${temp.name}, maxCap: $maxCap")
        }
    }


    /**
     * 写日志的task
     */

    fun write2File(file: File, buffer: CharBuffer, onFileWriteFinished: () -> Unit) {
        synchronized(buffer) {
            if (!file.exists()) {
                try {
                    file.createNewFile()
                    file.setWritable(true)
                } catch (e: IOException) {
                    LoggerUtils.i(MODULE_TAG, TAG, "----create logfile Exception ${file.name}")
                }
            }

            LoggerUtils.d(
                MODULE_TAG,
                TAG,
                " ${file.name}is going to FIO,tid${Thread.currentThread().id},buffPos ${buffer.position()} "
            )
//            if (buffer.isEmpty()) {
//                return
//            }
            var fw: FileWriter? = null
            try {
                LoggerUtils.d(
                    MODULE_TAG,
                    TAG,
                    file.name + "  FIO start with thread id:" + Thread.currentThread().id
                )
                try {
                    fw = FileWriter(file, true)
                } catch (e: FileNotFoundException) {
                    e.printStackTrace()
                    Log.e(MODULE_TAG, "file not found $e")
                    return
                }
                try {
                    fw.write(buffer.array(), 0, buffer.position())
                } catch (e: IOException) {
                    // java.io.IOException: write failed: ENOSPC (No space left on device)
                    LoggerUtils.d(
                        MODULE_TAG,
                        TAG,
                        " FIO write failed with ${e.message} with thread id ${Thread.currentThread().id}"
                    )
                    // return
                }
                fw.flush()
                LoggerUtils.d(
                    MODULE_TAG,
                    TAG,
                    " FIO write finished with thread id ${Thread.currentThread().id}"
                )
                onFileWriteFinished.invoke()
            } catch (e: Exception) {
                e.printStackTrace()
                LoggerUtils.d(MODULE_TAG, TAG, "Exception:$e")
                throw e
            } finally {
                try {
                    fw?.close()
                } catch (e: IOException) {
                    LoggerUtils.d(MODULE_TAG, TAG, "Exception:$e")
                }
                buffer.clear()
            }
        }
    }


    fun zipFile(source: File, size: Int, zip: File) {
        Log.e(MODULE_TAG, "zipFile start,${source.path}->${zip.path}")
        val ts = System.currentTimeMillis()
        val zpOs = FileOutputStream(zip)
        val zipOs = ZipOutputStream(zpOs)
        zipOs.setLevel(BEST_COMPRESSION)
        zipOs.putNextEntry(ZipEntry(source.name))
        val fis = FileInputStream(source)
        fis.channel.transferTo(0, size.toLong(), Channels.newChannel(zipOs))
        zipOs.finish()
        zipOs.closeEntry()
        fis.close()
        zpOs.close()
        zipOs.close()
        Log.e(MODULE_TAG, "zipFile $source finished->${zip.path} with size ${zip.length()},with ts cost:${System.currentTimeMillis() - ts} ms,tid: ${Thread.currentThread().id}")
    }


    fun write2Buffer(
        byteBuffer: CharBuffer,
        tempBuff: CharBuffer,
        charArray: CharArray,
        writeImmediately: Boolean
    ): Boolean {
        val charArraySize = charArray.size
        synchronized(byteBuffer) {
            val bufferCapacity = byteBuffer.capacity()
            val bufferPos = byteBuffer.position()
            val pos = byteBuffer.position() + charArraySize
            if (pos <= bufferCapacity) {
                byteBuffer.put(charArray)
                if (!writeImmediately) {
                    return true
                } else {

                }
            } else {
                if (bufferPos < bufferCapacity) {
                    val writePos = bufferCapacity - bufferPos
                    val offset = charArraySize - writePos
                    //部分放入Buff,多出的部分放入tempBuffer
                    Log.d(
                        MODULE_TAG,
                        "put partly string to backup buff,bufferCapacity=$bufferCapacity(${byteBuffer.capacity()}),bufferPos=$bufferPos(${byteBuffer.position()}),writePos=$writePos(${byteBuffer.remaining()}),charArraySize=$charArraySize,tid ${Thread.currentThread().id}"
                    )
                    byteBuffer.put(charArray, 0, writePos)
                    synchronized(tempBuff) {
                        if (tempBuff.position() + offset < tempBuff.capacity()) {
                            tempBuff.put(charArray, writePos, offset)
                            Log.d(
                                MODULE_TAG,
                                "put partly string to backup buff,bufferCapacity=$bufferCapacity,bufferPos=$bufferPos,tid ${Thread.currentThread().id}"
                            )
                        } else {
                            Log.e(
                                MODULE_TAG,
                                "put partly string to backup buff failed,abort this log !!!tid ${Thread.currentThread().id}"
                            )
                        }
                    }
                } else {
                    //buffer is full,then put str to temp buff
                    synchronized(tempBuff) {
                        val pos1 = tempBuff.position() + charArraySize
                        if (pos1 <= tempBuff.capacity()) {
                            tempBuff.put(charArray)
                            Log.e(
                                MODULE_TAG,
                                "put full string to backup buff,tid ${Thread.currentThread().id}"
                            )
                        } else {
                            Log.e(
                                MODULE_TAG,
                                "put full string to backup buff failed,abort this log !!!tid ${Thread.currentThread().id}"
                            )
                        }
                    }
                }

            }
        }
        return false
    }


}

enum class CutMode {
    STRICT,
    NORMAL
}