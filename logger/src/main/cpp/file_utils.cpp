
#include "include/file_utils.h"
#include "common.h"


char *getParentDir(char *&name) {
    string s = name;
    s = s.substr(0, s.find_last_of('/') + 1);
    return const_cast<char *>(s.data());
}



std::streampos fileSize(const char *filePath) {
    std::streampos fsize = 0;
    std::ifstream file(filePath, std::ios::binary);
    fsize = file.tellg();
    file.seekg(0, std::ios::end);
    fsize = file.tellg() - fsize;
    file.close();
    return fsize;
}


//int fsCopyPlus(char *const source, int size, const char *const cpyName) {
//    mode_t attr = S_IRWXU | S_IRWXG | S_IRWXO;
//    int fd = open(source, O_RDWR | O_CREAT, attr);
//    int cpyfd = open(cpyName, O_RDWR | O_CREAT, attr);
//    if (cpyfd == -1) {
//        LOGE(TAG, "file %s not exist %d", cpyName, cpyfd);
//    }
//    ftruncate(fd, size);
//    int len1 = getFileSize(cpyfd);
//    LOGI(TAG, "cpy finished ,-1before %d, ,%d,%s", len1, getFileSize(fd), cpyName);
//    fstream src(source, ios::binary | ios::ate);
//    fstream dst(cpyName, ios::app);
//    dst << src.rdbuf();
//    dst.flush();
//    dst.close();
//    src.close();
//    return len1;
//}


int fsCopy(int cacheFd, const char *const logPath) {
    LOGE(TAG, "fsCopy-> open  file  %d,%s", cacheFd, logPath);
    ::size_t len = getFileSize(cacheFd);
    int cpyfd = open(logPath, O_RDWR| O_CREAT, 0664);
    if (cpyfd == -1) {
        LOGE(TAG, "open cpy file failed %d,%s", cpyfd, logPath);
        return -1;
    }
    int cpyFileSize = getFileSize(cpyfd);

    lseek(cacheFd,0,SEEK_SET);
    lseek(cpyfd,cpyFileSize,SEEK_SET);

    LOGD(TAG, "fs copy,log size before append:%d + %d -> %d", cpyFileSize, len, cpyFileSize+len);
    int readPos = 0;
    ::size_t mRead;
    char *buff = static_cast<char *>(::malloc(4096));
    do {
        mRead = read(cacheFd, buff, 4096);
        if (mRead > 0) {
            readPos += mRead;
            write(cpyfd, buff, mRead);
        }
        if (readPos == len) break;
    } while (mRead > 0);

    std::free(buff);
    int cpySize = getFileSize(cpyfd);
    LOGE(TAG, "fyCopy,fsize:%d,write size:%d", cpySize,readPos);
    close(cpyfd);
    return cpySize;
}





time_t GetCurrentTimeMsec(){
    auto time = chrono::time_point_cast<chrono::milliseconds>(chrono::system_clock::now());
    time_t timestamp = time.time_since_epoch().count();
    return timestamp;
}



int trimFile(int fd, int size, const char *dstPath) {
    LOGI(TAG, "trimFile,open  file  %d,%s,%zu", fd, dstPath, size);
    ::size_t len = getFileSize(fd);
    int oversize=len-size;
    LOGD(TAG, "trimFile,file oversize %zu", oversize);
    int cpyfd = open(dstPath, O_RDWR | O_CREAT|O_APPEND, 0664);
    if (cpyfd == -1) {
        LOGE(TAG, "trimFile,open cpy file failed %d,%s", cpyfd, dstPath);
    }
    ints cpyFileSize = getFileSize(cpyfd);
    LOGD(TAG, "trimFile,log size before append:%zu + %zu = %zu", cpyFileSize, len, cpyFileSize + len);
    auto *source = static_cast<int8_t *>(mmap(nullptr, len, PROT_READ | PROT_WRITE, MAP_SHARED, fd,0));
    ints smode = len % PAGE_SIZE;
    ints spageOffset = smode > 0 ? len / PAGE_SIZE + 1 : len / PAGE_SIZE;
    ints mode = (cpyFileSize + len) % PAGE_SIZE;
    int pageOffset =
            mode > 0 ? (cpyFileSize + len) / PAGE_SIZE + 1 : (cpyFileSize + len) / PAGE_SIZE;
    ftruncate(cpyfd, pageOffset * PAGE_SIZE);
    auto *dest = static_cast<int8_t *>(mmap(nullptr, len, PROT_READ | PROT_WRITE, MAP_SHARED, cpyfd,
                                            (pageOffset - spageOffset) * PAGE_SIZE));
    memcpy(dest, source, len);
    LOGD(TAG, "trimFile,trim file success");
    munmap(dest, len);
    munmap(source, len);
    ints cpySize = getFileSize(cpyfd);
    close(cpyfd);
    if(oversize<0){
        LOGW(TAG,"invalid over size %d",oversize);
        oversize=0;
    }
    ints real = cpySize - oversize;
    if(oversize>0){
        ftruncate(cpyfd, real);
        LOGD(TAG, "trimFile,log size :%zu,%zu,%zu,%d,%d", real, cpySize, oversize,size,len);
    }

    return real;

//    int cpyfd = open(dstPath, O_RDWR |O_CREAT, 0664);
//    if (cpyfd == -1) {
//        LOGE(TAG, "open cpy file failed %d,%s", cpyfd, dstPath);
//    }
//    int cpyFileSize= getFileSize(cpyfd);
//    close(cpyfd);
//    return  copyFileFst(fd,0,size,dstPath,cpyFileSize);
}


int copyFileFst(int fd, int start, int len, const char *dst, int toStart) {
    LOGE(TAG, "copyFileFst(%d) from %d with size %d to %s with startPos %d", fd, start, len, dst,
         toStart);
    int dstFd = open(dst, O_RDWR | O_CREAT, 0664);
    if (dstFd == -1) {
        LOGE(TAG, "open cpy file failed %d,%s", dstFd, dst);
        return -1;
    }

    int dstFileSize = getFileSize(dstFd);
    int dstFileSizeLater = dstFileSize + len;
    LOGI(TAG, "copyFileFst->dst file size: %d + %d ->%d", dstFileSize, len, dstFileSizeLater);

    if (toStart % PAGE_SIZE > 0) {
        LOGE(TAG, "plz make sure  the toStart can be devided by 4096");
        return -1;
    }

    if (start % PAGE_SIZE) {
        LOGE(TAG, "plz make sure  the start can be devided by 4096");
        return -1;
    }

    int dstFileSizeLaterReal = dstFileSizeLater;
    if (dstFileSize < toStart || dstFileSize < toStart + len) {
        int dst_mode = (toStart + len) % PAGE_SIZE;
        int dst_pageOffset =
                dst_mode == 0 ? toStart + len / PAGE_SIZE : toStart + len / PAGE_SIZE + 1;
        dstFileSizeLaterReal = dst_pageOffset * PAGE_SIZE;
        LOGI(TAG, " dst file 扩容到 %d，原始值：%d,期望值%d", dstFileSizeLaterReal, dstFileSize,
             dstFileSizeLater);
        ftruncate(dstFd, dstFileSizeLaterReal);
    }

    int realFactor = len % PAGE_SIZE == 0 ? len / PAGE_SIZE : len / PAGE_SIZE + 1;
    realFactor = realFactor < 25 ? realFactor : 25;
    int mapSize = len < PAGE_SIZE ? PAGE_SIZE : PAGE_SIZE * realFactor;

    int mode = len % mapSize;
    int repeat = mode == 0 ? len / mapSize : len / mapSize + 1;
    int startTimes = 0;
    while (startTimes < repeat) {
        LOGI(TAG, "fstCpy mmp source offset %d,len %d->%d", start, len, startTimes);
        auto *source = static_cast<int8_t *>(mmap(nullptr, mapSize, PROT_READ | PROT_WRITE,
                                                  MAP_SHARED, fd, start + startTimes * mapSize));
        LOGV(TAG, "fstCpy mmp dst offset %d,len %d", toStart, len);
        auto *dest = static_cast<int8_t *>(mmap(nullptr, mapSize, PROT_READ | PROT_WRITE,
                                                MAP_SHARED, dstFd, toStart + startTimes * mapSize));

        LOGV(TAG, "fstCpy mmp finished source: %s ,dst %s", source, dest);

        if (source == MAP_FAILED || dest == MAP_FAILED) {
            LOGE(TAG, "mem map failed!!!");
            return -1;
        }
        memcpy(dest, source, mapSize);
        munmap(dest, mapSize);
        munmap(source, mapSize);
        startTimes++;
        LOGI(TAG, "fstCpy file success");
    }

    ints cpySize = getFileSize(dstFd);
    ints over = dstFileSizeLaterReal - dstFileSizeLater;
    ints real = cpySize - over;
    ftruncate(dstFd, real);
    close(dstFd);
    LOGE(TAG, "log size :%d,%d,%d", real, cpySize, over);
    return real;
}


size_t getFileSize(int fd) {
    struct stat info{};
    fstat(fd, &info);
    return info.st_size;
}


void FileMerge(char *filename) {
    char partname[NAME_LENGTH];
    int buffer[BUFFER_SIZE];
    int num = 0;
    FILE *fin, *fout = fopen(filename, "wb");
    if (fout == nullptr) {
        printf("Unable to create file.\n");
        exit(-2);
    }
    while (sprintf(partname, "part_%d", ++num) && (fin = fopen(partname, "rb")) != nullptr) {
        while (!feof(fin)) {
            ints cnt = fread(buffer, 1, 1024, fin);//1KB
            fwrite(buffer, 1, cnt, fout);
        }
        fclose(fin);
    }
    fclose(fout);
}

 void decompressFile(const char* fname,const char* outName)
{
    int const  fin = open(fname, O_RDWR , 0664);

    int  const fout = open(outName, O_RDWR|O_CREAT, 0664);
    if(fin<0||fout<0){
        LOGE(TAG,"open failed %d,%d",fin,fout);
        return;
    }


    size_t const buffInSize = ZSTD_DStreamInSize();
    void*  const buffIn  = malloc_orDie(buffInSize);
    size_t const buffOutSize = ZSTD_DStreamOutSize();  /* Guarantee to successfully flush at least one complete compressed block in all circumstances. */
    void*  const buffOut = malloc_orDie(buffOutSize);

    ZSTD_DCtx* const dctx = ZSTD_createDCtx();
    CHECK(dctx != nullptr, "ZSTD_createDCtx() failed!");

    /* This loop assumes that the input file is one or more concatenated zstd
     * streams. This example won't work if there is trailing non-zstd data at
     * the end, but streaming decompression in general handles this case.
     * ZSTD_decompressStream() returns 0 exactly when the frame is completed,
     * and doesn't consume input after the frame.
     */
    size_t const toRead = buffInSize;
    size_t mread;
    size_t lastRet = 0;
    int isEmpty = 1;
    while ( (mread = read(fin,buffIn,buffInSize)) ) {
        isEmpty = 0;
        ZSTD_inBuffer input = { buffIn, mread, 0 };
        /* Given a valid frame, zstd won't consume the last byte of the frame
         * until it has flushed all of the decompressed data of the frame.
         * Therefore, instead of checking if the return code is 0, we can
         * decompress just check if input.pos < input.size.
         */
        while (input.pos < input.size) {
            ZSTD_outBuffer output = { buffOut, buffOutSize, 0 };
            /* The return code is zero if the frame is complete, but there may
             * be multiple frames concatenated together. Zstd will automatically
             * reset the context when a frame is complete. Still, calling
             * ZSTD_DCtx_reset() can be useful to reset the context to a clean
             * state, for instance if the last decompression call returned an
             * error.
             */
            size_t const ret = ZSTD_decompressStream(dctx, &output , &input);
            CHECK_ZSTD(ret);
            write(fout,buffOut,output.pos);
            lastRet = ret;
        }
    }

    if (isEmpty) {
        fprintf(stderr, "input is empty\n");
        exit(1);
    }

    if (lastRet != 0) {
        /* The last return value from ZSTD_decompressStream did not end on a
         * frame, but we reached the end of the file! We assume this is an
         * error, and the input was truncated.
         */
        fprintf(stderr, "EOF before end of stream: %zu\n", lastRet);
        exit(1);
    }

    ZSTD_freeDCtx(dctx);
    close(fin);
    close(fout);
    free(buffIn);
    free(buffOut);
}

 void compressFile(const char* fname, const char* outName, int cLevel,
                               int nbThreads)
{

    LOGD(TAG,"[file_utils][compressFile：%s -> %s",fname,outName);
    fprintf (stderr, "Starting compression of %s with level %d, using %d threads\n",fname, cLevel, nbThreads);

    /* Open the input and output files. */
    int const  fin = open(fname, O_RDWR , 0664);

    int  const fout = open(outName, O_RDWR|O_CREAT, 0664);
    if(fin<0||fout<0){
        LOGE(TAG,"open failed %d,%d",fin,fout);
        return;
    }
    int insize = getFileSize(fin);
    int outsize = getFileSize(fout);

    LOGE(TAG,"open success %d,%d,%d,%d",fin,insize,fout,outsize);

    /* Create the input and output buffers.
     * They may be any size, but we recommend using these functions to size them.
     * Performance will only suffer significantly for very tiny buffers.
     */
    size_t const  buffInSize = ZSTD_CStreamInSize();
    void* const buffIn  = malloc(buffInSize);
    size_t const buffOutSize = ZSTD_CStreamOutSize();
    void*  const buffOut = malloc(buffOutSize);

    /* Create the context. */
    ZSTD_CCtx* const cctx = ZSTD_createCCtx();
    CHECK(cctx != nullptr, "ZSTD_createCCtx() failed!");
    LOGD(TAG,"compress create ctx %d",cctx);

    /* Set any parameters you want.
     * Here we set the compression level, and enable the checksum.
     */
    CHECK_ZSTD( ZSTD_CCtx_setParameter(cctx, ZSTD_c_compressionLevel, cLevel) );
    CHECK_ZSTD( ZSTD_CCtx_setParameter(cctx, ZSTD_c_checksumFlag, 1) );
    if (nbThreads > 1) {
        size_t const r = ZSTD_CCtx_setParameter(cctx, ZSTD_c_nbWorkers, nbThreads);
        if (ZSTD_isError(r)) {
            fprintf (stderr, "Note: the linked libzstd library doesn't support multithreading. "
                             "Reverting to single-thread mode. \n");
        }
    }

    /* This loop read from the input file, compresses that entire chunk,
     * and writes all output produced to the output file.
     */
    size_t const toRead = buffInSize;
    LOGD(TAG,"[file_utils][compress start]");
    ssize_t  mRead;
    for (;;) {
        mRead=read(fin,buffIn,buffInSize);
      //  LOGD(TAG,"[file_utils][compress,do read %d]",mRead);
        /* Select the flush mode.
         * If the read may not be finished (read == toRead) we use
         * ZSTD_e_continue. If this is the last chunk, we use ZSTD_e_end.
         * Zstd optimizes the case where the first flush mode is ZSTD_e_end,
         * since it knows it is compressing the entire source in one pass.
         */
        int const lastChunk = (mRead < toRead);
        ZSTD_EndDirective const mode = lastChunk ? ZSTD_e_end : ZSTD_e_continue;
        /* Set the input buffer to what we just read.
         * We compress until the input buffer is empty, each time flushing the
         * output.
         */
        ZSTD_inBuffer input = { buffIn, static_cast<size_t>(mRead), 0 };
        int finished;
        do {
           // LOGD(TAG,"[file_utils][compress,do]");
            /* Compress into the output buffer and write all of the output to
             * the file so we can reuse the buffer next iteration.
             */
            ZSTD_outBuffer output = { buffOut, buffOutSize, 0 };
            size_t const remaining = ZSTD_compressStream2(cctx, &output , &input, mode);
            CHECK_ZSTD(remaining);
            write(fout,buffOut,output.pos);
//            fwrite_orDie(buffOut, output.pos, fout);
            /* If we're on the last chunk we're finished when zstd returns 0,
             * which means its consumed all the input AND finished the frame.
             * Otherwise, we're finished when we've consumed all the input.
             */
            finished = lastChunk ? (remaining == 0) : (input.pos == input.size);
         //   LOGD(TAG,"[file_utils][do once %d vs %d]",input.pos,input.size);
        } while (!finished);
        CHECK(input.pos == input.size,
              "Impossible: zstd only returns 0 when the input is completely consumed!");

        if (lastChunk) {
            LOGD(TAG,"[file_utils][last chunk]");
            break;
        }
    }
    LOGD(TAG,"[file_utils][finished]");
    ZSTD_freeCCtx(cctx);
    close(fin);
    close(fout);
    free(buffIn);
    free(buffOut);
    LOGD(TAG,"[file_utils][free resource]");
}


static char* createOutFilename_orDie(const char* filename)
{
    size_t const inL = strlen(filename);
    size_t const outL = inL + 5;
    char* const outSpace = static_cast<char *const>(malloc_orDie(outL));
    memset(outSpace, 0, outL);
    strcat(outSpace, filename);
    strcat(outSpace, ".zst");
    return (char*)outSpace;
}



//int main(){
//    printf("Selection function:\n1.FileSplit\n2.FileMarge\n0.quit\n>>");
//    int choose=0;
//    while(scanf("%d",&choose) && choose){
//        while(getchar()!='\n') continue;
//        char filename[NAME_LENGTH];
//        if(choose==1){
//            printf("Please enter the name of the target file>>\n");
//            gets(filename);
//            FILE *fin=fopen(filename,"rb");
//            printf("Please enter the size(MB) of each block>>\n");
//            int size=0;
//            scanf("%d",&size);
//            printf("Wait...\n");
//            FileSplit(fin,size);
//            fclose(fin);
//            printf("Finish.\n");
//        }
//        else if(choose==2){
//            printf("Please enter the name of the output file>>\n");
//            gets(filename);
//            printf("Wait...\n");
//            FileMerge(filename);
//            printf("Finish.\n");
//        }
//        printf("Continue selecting function\n>>");
//    }
//    return 0;
//}